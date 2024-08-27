#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../legalizer/legalizer.h"
#include "../config/config_manager.h"
#include "../estimator/lib_cell_evaluator.h"
#include "../legalizer/utilization.h"
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
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();
    timer::Timer &timer = timer::Timer::get_instance();

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
            // "Utilization change"
            std::cout<<"Try cluster:";
            for(int cid : cluster){
                const circuit::Cell &cell = netlist.get_cell(cid);
                int bits = cell.get_bits();
                std::cout<<cid<<"("<<bits<<") ";
            }
            std::cout<<std::endl;
            
            
            double remove_utilization_cost_change =  utilization_calculator.remove_cells_cost_change(cluster);
            double cluster_cost_change = ff_libcells_cost_manager.get_cluster_cost_change(cluster);            
            // "Netlist and legalizer change"
            int cluster_res = netlist.cluster_cells_without_check(cluster);

            if(cluster_res == 0){
                std::cout<<"cluster and legal success"<<std::endl;
            }else if(cluster_res == 1){
                std::cout<<"Cluster fail due to legal rollback this cluster"<<std::endl;
                // "Netlist and legalizer rollback"
                solution_manager.rollack_clustering_res_using_best_solution_skip_timer(cluster);
                // "Utilization recover"
                utilization_calculator.add_cells(cluster);
                continue;
            }else if(cluster_res == 2){
                // swap_ff but it's already the best ff
                // "Netlist and legalizer have no change"
                // "Utilization recover"
                utilization_calculator.add_cells(cluster);
                continue;
            }

            int parent_cell_id = cluster[0];
            // check utilization: if there are new bins overflow
            // "Utilization change"            
            double add_utilization_cost_change = utilization_calculator.add_cell_cost_change(parent_cell_id);
            double utilization_cost_change = add_utilization_cost_change + remove_utilization_cost_change;
            
            // timer: update_timing and check calculate cost change
            // "Timing change"
            std::unordered_set<int> affected_timing_cells_id_set;
            double timing_cost_change = timer.calculate_timing_cost_after_cluster(parent_cell_id,affected_timing_cells_id_set);
            const std::vector<int> &affected_timing_cells_id = std::vector<int>(affected_timing_cells_id_set.begin(),affected_timing_cells_id_set.end());
            
            std::cout<<"Clustered res: cluster_cost_change:"<<cluster_cost_change<<" utilization_cost_change:"<<utilization_cost_change<<" timing_cost_change:"<<timing_cost_change<<std::endl;
            double cost_change = cluster_cost_change + utilization_cost_change + timing_cost_change;
            if(cost_change <= 0){
                std::cout<<"Good cluster: benefit:"<<cost_change<<std::endl;                
                // commit solution
                // "Update cost"   (power,area cost/ timing cost)                
                cost_calculator.update_cells_cost_after_clustering(cluster,affected_timing_cells_id);            
                // DEBUG START
                double origin_best_cost = solution_manager.get_best_solution().get_cost();
                double new_cost = cost_calculator.get_cost();
                std::cout<<"New cost:"<<new_cost<<" Best cost:"<<origin_best_cost<<std::endl;
                if(new_cost > origin_best_cost){
                    std::cout<<"Something wrong"<<std::endl;
                }       
                // DEBUG END             

                // "Sync best solution: update netlist"                
                solution_manager.update_best_solution_after_clustering(cluster,new_cost);
            }else{
                std::cout<<"Bad cluster: benefit:"<<cost_change<<std::endl;
                utilization_calculator.remove_cell(cluster[0]);
                std::cout<<"Rollback utilization finish"<<std::endl;
                // "Rollback netlist -> (legalizer, timer)"
                solution_manager.rollack_clustering_res_using_best_solution(cluster);
                std::cout<<"Rollback netlist finish"<<std::endl;
                // "Rollback cost"
                cost_calculator.rollack_clustering_res(cluster);
                std::cout<<"Rollback cluster cost finish"<<std::endl;
                cost_calculator.rollack_timer_res(affected_timing_cells_id);
                std::cout<<"Rollback timer cost finish"<<std::endl;
                // "Rollback Utilization"
                utilization_calculator.add_cells(cluster);
                std::cout<<"Rollback utilization2 finish"<<std::endl;
            }


            /*
            double new_cost = cost_calculator.get_cost();
            double best_cost = solution_manager.get_best_solution().get_cost();

            std::cout<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;            
            if (new_cost < best_cost) {
                cost_calculator.calculate_cost_update_by_cells_id_for_cluster(cluster);
                solution_manager.update_best_solution_by_cells_id(cluster,new_cost);
                std::cout<<"New cost is better than best cost"<<std::endl;
            }else{
                int parent_cell_id = cluster[0];
                 const circuit::Cell &cell = netlist.get_cell(parent_cell_id);
                int bits = cell.get_bits();
                int new_lib_cell_id = netlist.get_cell(parent_cell_id).get_lib_cell_id();
                const design::LibCell &lib_cell = design::Design::get_instance().get_lib_cells().at(cell.get_lib_cell_id());
                const std::string &lib_cell_name = lib_cell.get_name();
                double lib_cell_cost = ff_libcells_cost_manager.get_lib_cell_cost(new_lib_cell_id);
                std::cout<<"Clustered:"<<parent_cell_id<<"["+lib_cell_name+"("<<bits<<")] lib_cell_cost:"<<lib_cell_cost<<std::endl;

                solution_manager.rollack_best_solution_by_cells_id(cluster);
                cost_calculator.calculate_cost_rollback_by_cells_id(cluster);
                std::cout<<"New cost is worse than best cost rollback why???"<<std::endl;
                std::cout<<"Try cluster:";
                for(int cell_id : cluster){
                    const circuit::Cell &cell = netlist.get_cell(cell_id);
                    int bits = cell.get_bits();
                    int new_lib_cell_id = cell.get_lib_cell_id();
                    const design::LibCell &lib_cell = design::Design::get_instance().get_lib_cells().at(new_lib_cell_id);
                    const std::string &lib_cell_name = lib_cell.get_name();
                    double lib_cell_cost = ff_libcells_cost_manager.get_lib_cell_cost(new_lib_cell_id);
                    std::cout<<"Rollback:"<<cell_id<<"["+lib_cell_name+"("<<bits<<")] lib_cell_cost:"<<lib_cell_cost<<" ";
                }
                std::cout<<std::endl;
            }
            */
            if(runtime.is_timeout()){
                break;
            }
        } 
        if(runtime.is_timeout()){
            break;
        }         
    }
    utilization_calculator.print();
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