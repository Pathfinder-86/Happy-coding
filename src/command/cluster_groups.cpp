#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../legalizer/legalizer.h"
#include "../config/config_manager.h"
#include "../estimator/lib_cell_evaluator.h"
#include <algorithm>
#include <limits> 
#include <cmath>
#include <unordered_map>
#include "../runtime/runtime.h"
namespace command{

std::vector<std::vector<int>> divide_into_matching_cluster_by_y_order(const std::vector<int> &,int ,const std::vector<int> &,const std::vector<int> &);

void CommandManager::kmeans_cluster(){
    std::cout << "INIT Kmeans_cluster" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    int mid_bits_of_lib = ff_libcells_cost_manager.get_mid_bits_of_lib();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();
    
    
    std::vector<std::pair<int,int>> clk_groups_id_size;
    for(const auto &clk_group_id_ff_cell_ids : clk_group_id_to_ff_cell_ids){
        clk_groups_id_size.push_back(std::make_pair(clk_group_id_ff_cell_ids.first,clk_group_id_ff_cell_ids.second.size()));
    }
    std::sort(clk_groups_id_size.begin(),clk_groups_id_size.end(),[](const std::pair<int,int> &a, const std::pair<int,int> &b){
        return a.second > b.second;
    });
    // forecah clk group
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();   

    for(int i=0;i<clk_groups_id_size.size();i++){
        int clk_group_id = clk_groups_id_size[i].first;
        const std::unordered_set<int> &ff_cell_ids = clk_group_id_to_ff_cell_ids.at(clk_group_id);


        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            int bits = cell.get_bits();
            bits_sum[bits] += bits;
            bits_cell_ids[bits].push_back(cell_id);                        
        }

               
        std::unordered_map<int,std::vector<int>> bits_clustering_map;
        for(auto &it : bits_sum){
            int bits = it.first;
            bits_clustering_map[bits] = std::vector<int>(N,0);
        }

        for(const auto &it : bits_sum){
            int bits = it.first;
            int sum = it.second;
            int idx = 0;
            while(sum > 0){
                if(best_libcell_sorted_by_bits[idx] > sum){
                    idx++;
                }else{
                    int num = sum / best_libcell_sorted_by_bits[idx];
                    sum -= num * best_libcell_sorted_by_bits[idx];
                    bits_clustering_map[bits][idx]+=num;
                }

            }
        }

        std::vector<std::vector<int>> clustering_res;

        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            for(const auto &cluster : matching_clusters){
                clustering_res.push_back(cluster);
            }
        }

        for(const auto &cluster : clustering_res){

            int cluster_res = netlist.cluster_cells_without_check(cluster);

            if(cluster_res == 0){
                std::cout<<"Cluster success"<<std::endl;
            }else if(cluster_res == 1){
                std::cout<<"Cluster fail due to legal rollback this cluster"<<std::endl;
                solution_manager.rollack_best_solution_by_cells_id(cluster);
                cost_calculator.calculate_cost_rollback_by_cells_id(cluster);
                continue;
            }else if(cluster_res == 2){
                std::cout<<"single bit don't change"<<std::endl;
                continue;
            }

            cost_calculator.calculate_cost_update_by_cells_id(cluster);
            double new_cost = cost_calculator.get_cost();
            double best_cost = solution_manager.get_best_solution().get_cost();

            std::cout<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;            
            if (new_cost < best_cost) {
                solution_manager.update_best_solution_by_cells_id(cluster,new_cost);
                std::cout<<"New cost is better than best cost"<<std::endl;
            }else{
                solution_manager.rollack_best_solution_by_cells_id(cluster);
                cost_calculator.calculate_cost_rollback_by_cells_id(cluster);
                std::cout<<"New cost is worse than best cost rollback (timing factor)"<<std::endl;
            }
            if(runtime.is_timeout()){
                break;
            }
        } 
        if(runtime.is_timeout()){
            break;
        } 
        
        /* 
        clk group cluster commit STUCK
        int cluster_clk_group_res = netlist.cluster_clk_group(clustering_res);
        if(cluster_clk_group_res == 0){
            std::cout<<"Cluster success"<<std::endl;
        }else if(cluster_clk_group_res == -1){
            std::cout<<"Cluster fail due to legal rollback"<<std::endl;
            solution_manager.switch_to_best_solution();
            continue;
        }

        cost_calculator.calculate_cost();
        double new_cost = cost_calculator.get_cost();
        double best_cost = solution_manager.get_best_solution().get_cost();
        std::cout<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;
        if (new_cost < best_cost) {
            solution_manager.keep_best_solution();
            std::cout<<"New cost is better than best cost"<<std::endl;
        }else{
            solution_manager.switch_to_best_solution();
            std::cout<<"New cost is worse than best cost rollback"<<std::endl;
        }
        if(runtime.is_timeout()){
            break;
        }
        */
    }
}

std::vector<std::vector<int>> divide_into_matching_cluster_by_y_order(const std::vector<int> &cells_id,int cell_bits,const std::vector<int> &matching_res,const std::vector<int> &best_libcell_sorted_by_bits){    
    std::vector<std::vector<int>> nodes; // idx, cell.get_x(), cell.get_y()
    // sort by y and then sort by x

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int cell_id : cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        nodes.push_back(std::vector<int>{cell_id,cell.get_x(),cell.get_y()});        
    }

    std::sort(nodes.begin(), nodes.end(), [](const std::vector<int>& node1, const std::vector<int>& node2) {
        if (node1[2] == node2[2]) {
            return node1[1] < node2[1];
        }
        return node1[2] < node2[2];
    });
    
    std::vector<std::vector<int>> clusters;
    int idx = 0;
    for(int i=0;i<matching_res.size();i++){
        int num = matching_res[i];
        int bits = best_libcell_sorted_by_bits[i];
        if(num == 0){
            continue;
        }
        
        std::vector<int> cluster;
        int cluster_num = bits / cell_bits;
        for(int j=0;j<num;j++){
            for(int k=0;k<cluster_num;k++){
                cluster.push_back(nodes[idx][0]);
                idx++;
            }
            clusters.push_back(cluster);
            cluster.clear();
        }        
    }
    
    return clusters;
}


}