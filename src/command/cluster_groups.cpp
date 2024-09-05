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
#include "../circuit/original_netlist.h"
#include <iomanip>
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
    const design::Design &design = design::Design::get_instance();

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
        int count = 0;
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            for(const auto &cluster : matching_clusters){
                clustering_res.push_back(cluster);
            }
        }
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        for(const auto &cluster : clustering_res){
            // "Utilization change"
            std::cout<<"Try cluster:";
            for(int cid : cluster){
                const circuit::Cell &cell = netlist.get_cell(cid);
                int bits = cell.get_bits();
                std::cout<<cid<<"("<<bits<<") ";
            }
            std::cout<<std::endl;
            
            for(int cid : cluster){
                int original_cell_id = original_netlist.get_original_cell_id(cid);
                const std::string &cell_name = original_netlist.get_cell_name(original_cell_id);
                std::cout<<cell_name<<" ";
            }
            std::cout<<std::endl;

            
            double remove_utilization_cost_change =  utilization_calculator.remove_cells_cost_change(cluster);
            double cluster_cost_change = ff_libcells_cost_manager.get_cluster_cost_change(cluster);            
            // "Netlist and legalizer change"
            int cluster_res = netlist.cluster_cells_with_legal(cluster);

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
            std::cout<<"utilization_cost Finish"<<std::endl;

            // timer: update_timing and check calculate cost change
            // "Timing change"
            double clustered_cost_change = 0.0;            
            for(int i = 1;i<cluster.size();i++){               
                int cell_id = cluster[i];
                circuit::Cell &cell = netlist.get_mutable_cell(cell_id);                     
                clustered_cost_change += -cell.get_tns() * design.get_timing_factor();                
                cell.set_tns(0.0);
            }
            std::vector<int> affected_timing_cells_id;
            double affected_timing_cost = timer.calculate_timing_cost_after_cluster(cluster,affected_timing_cells_id);
            std::cout<<"Timing cost Finish"<<std::endl;
            
            double timing_cost_change = affected_timing_cost + clustered_cost_change;
            std::cout<<"Timing cost change:"<<timing_cost_change<<" clustered_cost_change:"<<clustered_cost_change<<" affected_timing_cost:"<<affected_timing_cost<<std::endl;                        
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
                if(new_cost > origin_best_cost){
                    std::cout<<"Something wrong"<<std::endl;
                }       
                // DEBUG END             

                // "Sync best solution: update netlist"                
                solution_manager.update_best_solution_after_clustering(cluster,new_cost);
                // "Keep timing"
                solution_manager.keep_timing(affected_timing_cells_id);                
                // legal                
                std::cout<<"Cost: "<<cost_calculator.get_timing_cost()<<" "<<cost_calculator.get_power_cost()<<" "<<cost_calculator.get_area_cost()<<" "<<cost_calculator.get_utilization_cost()<<std::endl;
                count++;

                std::cout<<"COST/RUNTIME "<<new_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
                

            }else{
                std::cout<<"Bad cluster: benefit:"<<cost_change<<std::endl;
                utilization_calculator.remove_cell(cluster[0]);
                std::cout<<"Rollback utilization finish"<<std::endl;
                // "Rollback netlist -> (legalizer, timer)"
                solution_manager.rollack_clustering_res_using_best_solution(cluster,affected_timing_cells_id);
                std::cout<<"Rollback netlist finish"<<std::endl;
                // "Rollback cost"
                //cost_calculator.rollack_clustering_res(cluster);
                std::cout<<"Rollback cluster cost finish"<<std::endl;
                //cost_calculator.rollack_timer_res(affected_timing_cells_id);
                std::cout<<"Rollback timer cost finish"<<std::endl;
                // "Rollback Utilization"
                utilization_calculator.add_cells(cluster);
                std::cout<<"Rollback utilization2 finish"<<std::endl;
                double current_cost = cost_calculator.get_cost();
                double best_cost = solution_manager.get_best_solution().get_cost();
                if(current_cost != best_cost){
                    std::cout<<"Rollback cost is wrong"<<std::endl;
                    std::cout<<"Cost: "<<cost_calculator.get_timing_cost()<<" "<<cost_calculator.get_power_cost()<<" "<<cost_calculator.get_area_cost()<<" "<<cost_calculator.get_utilization_cost()<<std::endl;
                    std::cout<<"diff:"<<current_cost-best_cost<<" current_cost:"<<current_cost<<" best_cost:"<<best_cost<<std::endl;
                }
            }
            double new_cost = cost_calculator.get_cost();
            cost_calculator.print_cost();
            double best_cost = solution_manager.get_best_solution().get_cost();
            std::cout << std::fixed << std::setprecision(5)<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;
            solution_manager.keep_cost(new_cost); 
            if(runtime.is_timeout()){
                break;
            }
        }       
        if(runtime.is_timeout()){
            break;
        }         
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


void CommandManager::clustering_clean_solution(){
    std::cout << "INIT Kmeans_cluster" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();
    timer::Timer &timer = timer::Timer::get_instance();
    const design::Design &design = design::Design::get_instance();

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
        int count = 0;
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            for(const auto &cluster : matching_clusters){
                clustering_res.push_back(cluster);
            }
        }
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        for(const auto &cluster : clustering_res){
            // "Utilization change"
            std::cout<<"Try cluster:";
            for(int cid : cluster){
                const circuit::Cell &cell = netlist.get_cell(cid);
                int bits = cell.get_bits();
                std::cout<<cid<<"("<<bits<<") ";
            }
            std::cout<<std::endl;
            
            for(int cid : cluster){
                int original_cell_id = original_netlist.get_original_cell_id(cid);
                const std::string &cell_name = original_netlist.get_cell_name(original_cell_id);
                std::cout<<cell_name<<" ";
            }
            std::cout<<std::endl;

            
            double remove_utilization_cost_change =  utilization_calculator.remove_cells_cost_change(cluster);
            double cluster_cost_change = ff_libcells_cost_manager.get_cluster_cost_change(cluster);            
            // "Netlist and legalizer change"
            int cluster_res = netlist.cluster_cells_with_legal(cluster);

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
            std::cout<<"utilization_cost Finish"<<std::endl;

            // timer: update_timing and check calculate cost change
            // "Timing change"
            double clustered_cost_change = 0.0;            
            for(int i = 1;i<cluster.size();i++){               
                int cell_id = cluster[i];
                circuit::Cell &cell = netlist.get_mutable_cell(cell_id);                     
                clustered_cost_change += -cell.get_tns() * design.get_timing_factor();                
                cell.set_tns(0.0);
            }
            std::vector<int> affected_timing_cells_id;
            double affected_timing_cost = timer.calculate_timing_cost_after_cluster(cluster,affected_timing_cells_id);
            std::cout<<"Timing cost Finish"<<std::endl;
            
            double timing_cost_change = affected_timing_cost + clustered_cost_change;
            std::cout<<"Timing cost change:"<<timing_cost_change<<" clustered_cost_change:"<<clustered_cost_change<<" affected_timing_cost:"<<affected_timing_cost<<std::endl;                        
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
                if(new_cost > origin_best_cost){
                    std::cout<<"Something wrong"<<std::endl;
                }       
                // DEBUG END             

                // "Sync best solution: update netlist"                
                solution_manager.update_best_solution_after_clustering(cluster,new_cost);
                // "Keep timing"
                solution_manager.keep_timing(affected_timing_cells_id);                
                // legal                
                std::cout<<"Cost: "<<cost_calculator.get_timing_cost()<<" "<<cost_calculator.get_power_cost()<<" "<<cost_calculator.get_area_cost()<<" "<<cost_calculator.get_utilization_cost()<<std::endl;
                count++;

                std::cout<<"COST/RUNTIME "<<new_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
                

            }else{
                std::cout<<"Bad cluster: benefit:"<<cost_change<<std::endl;
                utilization_calculator.remove_cell(cluster[0]);
                std::cout<<"Rollback utilization finish"<<std::endl;
                // "Rollback netlist -> (legalizer, timer)"
                solution_manager.rollack_clustering_res_using_best_solution(cluster,affected_timing_cells_id);
                std::cout<<"Rollback netlist finish"<<std::endl;
                // "Rollback cost"
                //cost_calculator.rollack_clustering_res(cluster);
                std::cout<<"Rollback cluster cost finish"<<std::endl;
                //cost_calculator.rollack_timer_res(affected_timing_cells_id);
                std::cout<<"Rollback timer cost finish"<<std::endl;
                // "Rollback Utilization"
                utilization_calculator.add_cells(cluster);
                std::cout<<"Rollback utilization2 finish"<<std::endl;
                double current_cost = cost_calculator.get_cost();
                double best_cost = solution_manager.get_best_solution().get_cost();
                if(current_cost != best_cost){
                    std::cout<<"Rollback cost is wrong"<<std::endl;
                    std::cout<<"Cost: "<<cost_calculator.get_timing_cost()<<" "<<cost_calculator.get_power_cost()<<" "<<cost_calculator.get_area_cost()<<" "<<cost_calculator.get_utilization_cost()<<std::endl;
                    std::cout<<"diff:"<<current_cost-best_cost<<" current_cost:"<<current_cost<<" best_cost:"<<best_cost<<std::endl;
                }
            }
            double new_cost = cost_calculator.get_cost();
            cost_calculator.print_cost();
            double best_cost = solution_manager.get_best_solution().get_cost();
            std::cout << std::fixed << std::setprecision(5)<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;
            solution_manager.keep_cost(new_cost); 
            if(runtime.is_timeout()){
                break;
            }
        }       
        if(runtime.is_timeout()){
            break;
        }         
    }
}


void CommandManager::faster_clustering(){
    std::cout << "INIT faster_clustering" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();


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
        int count = 0;
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            for(const auto &cluster : matching_clusters){
                clustering_res.push_back(cluster);
            }
        }

        // utilization remove all
        for(const auto &cluster : clustering_res){
            utilization_calculator.remove_cells(cluster);
        }

        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }

        // legalizer
        legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
        for(const auto &cluster : clustering_res){
            int parent_id = cluster[0];
            legalizer.replacement_cell(parent_id);
            for(int i=1;i<static_cast<int>(cluster.size());i++){        
                legalizer.remove_cell(cluster[i]);        
            }
        }

        if( legalizer.legalize() == false){
            // rollback
            std::cout<<"Legal fail"<<std::endl;
            solution_manager.switch_to_best_solution();
            continue;
        }

        // timer
        for(const auto &cluster : clustering_res){
            int parent_id = cluster[0];
            timer.update_timing_and_cells_tns(parent_id);
        }

        // utilization add

        for(const auto &cluster : clustering_res){
            int parent_id = cluster[0];
            utilization_calculator.add_cell(parent_id);
        }
        utilization_calculator.update_overflow_bins();
        
        // cost update
        cost_calculator.calculate_cost();

        // compare with best solution 

        double current_cost = cost_calculator.get_cost();
        double best_cost = solution_manager.get_best_solution().get_cost();
        std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
        if(current_cost < best_cost){
            std::cout<<"Keep best solution"<<std::endl;
            std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
            solution_manager.keep_best_solution();
        }else{
            std::cout<<"rollback to best solution"<<std::endl;
            solution_manager.switch_to_best_solution();
        }
    
        if(runtime.is_timeout()){
            break;
        }         
    }
}



bool CommandManager::extremely_fast_clustering(){
    std::cout << "INIT extremely_fast_clustering" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    

    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();


    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();   

    int not_in_clk_group = 0;
    for(auto &cell : netlist.get_mutable_cells()){
        int cell_id = cell.get_id();
        if(netlist.get_clk_group_id(cell_id) == -1){            
            not_in_clk_group++;
            int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
            int current_lib_cell_id = cell.get_lib_cell_id();
            if(best_lib_cell_id == -1 || current_lib_cell_id == best_lib_cell_id){
                continue;
            }
            netlist.swap_ff(cell_id,best_lib_cell_id);
        }
    }
    std::cout<<"Not in clk group:"<<not_in_clk_group<<std::endl;


    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        const std::unordered_set<int> &ff_cell_ids = it.second;

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

        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }        
    }

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all() == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // skip timer?
    timer::Timer &timer = timer::Timer::get_instance();
    for(int cell_id : netlist.get_sequential_cells_id()){
        timer.update_timing_and_cells_tns(cell_id);
    }

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}


bool CommandManager::extremely_fast_clustering_limit_high_bits(){
    std::cout << "INIT extremely_fast_clustering" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    

    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();

    int mid_bits_of_lib = ff_libcells_cost_manager.get_mid_bits_of_lib();    
    std::cout<<"Mid bits of lib:"<<mid_bits_of_lib<<std::endl;
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    std::vector<int> best_libcell_sorted_by_bits_limit_high;
    for(int bits : best_libcell_sorted_by_bits){
        if(bits <= mid_bits_of_lib){
            std::cout<<"Allow bits:"<<bits<<std::endl;
            best_libcell_sorted_by_bits_limit_high.push_back(bits);
        }
    }

    const int N = best_libcell_sorted_by_bits_limit_high.size();   

    int not_in_clk_group = 0;
    for(auto &cell : netlist.get_mutable_cells()){
        int cell_id = cell.get_id();
        if(netlist.get_clk_group_id(cell_id) == -1){            
            not_in_clk_group++;
            int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
            int current_lib_cell_id = cell.get_lib_cell_id();
            if(best_lib_cell_id == -1 || current_lib_cell_id == best_lib_cell_id){
                continue;
            }
            netlist.swap_ff(cell_id,best_lib_cell_id);
        }
    }
    std::cout<<"Not in clk group:"<<not_in_clk_group<<std::endl;


    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> ff_cell_ids_limit_high;
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(cell.get_bits() <= mid_bits_of_lib){
                ff_cell_ids_limit_high.insert(cell_id);
            }else{
                clustering_res.push_back(std::vector<int>{cell_id});
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : ff_cell_ids_limit_high){
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
                if(best_libcell_sorted_by_bits_limit_high[idx] > sum){
                    idx++;
                }else{
                    int num = sum / best_libcell_sorted_by_bits_limit_high[idx];
                    sum -= num * best_libcell_sorted_by_bits_limit_high[idx];
                    bits_clustering_map[bits][idx]+=num;
                }

            }
        }
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits_limit_high);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    std::cout<<"Legalizer start"<<std::endl;
    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all() == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // skip timer?
    timer::Timer &timer = timer::Timer::get_instance();
    for(int cell_id : netlist.get_sequential_cells_id()){
        timer.update_timing_and_cells_tns(cell_id);
    }

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}

bool CommandManager::clustering_exist_noncritical_q_pins_cells_limit_high_bits(){
    std::cout << "INIT clustering_exist_noncritical_q_pins_cells_limit_high_bits" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    timer.collect_non_critical_ffs_id();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    std::cout<<"Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<sequential_cells_id.size()<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/sequential_cells_id.size()<<std::endl;

    
    // clustering non critical cells
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();

    
    int mid_bits_of_lib = ff_libcells_cost_manager.get_mid_bits_of_lib();        
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    std::vector<int> best_libcell_sorted_by_bits_limit_high;
    for(int bits : best_libcell_sorted_by_bits){
        if(bits <= mid_bits_of_lib){            
            best_libcell_sorted_by_bits_limit_high.push_back(bits);
        }
    }
    const int N = best_libcell_sorted_by_bits_limit_high.size();

   
    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::cout<<"Clustering"<<std::endl;
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> ff_cell_ids_limit_high_and_noncritical;
        
        // swap high bits and critical cells to best libcell
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(cell.get_bits() <= mid_bits_of_lib && noncritical_q_pins_cells.find(cell_id) != noncritical_q_pins_cells.end()){
                ff_cell_ids_limit_high_and_noncritical.insert(cell_id);
            }else{
                int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
                int current_lib_cell_id = cell.get_lib_cell_id();
                if(current_lib_cell_id == best_lib_cell_id){
                    continue;
                }
                netlist.swap_ff(cell_id,best_lib_cell_id);
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : ff_cell_ids_limit_high_and_noncritical){
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
                if(best_libcell_sorted_by_bits_limit_high[idx] > sum){
                    idx++;
                }else{
                    int num = sum / best_libcell_sorted_by_bits_limit_high[idx];
                    sum -= num * best_libcell_sorted_by_bits_limit_high[idx];
                    bits_clustering_map[bits][idx]+=num;
                }

            }
        }
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits_limit_high);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }
        std::cout<<"Banking"<<std::endl;
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all() == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer
    for(int cell_id : netlist.get_sequential_cells_id()){
        timer.update_timing_and_cells_tns(cell_id);
    }

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}


bool CommandManager::clustering_exist_noncritical_q_pins_cells(){
    std::cout << "INIT clustering_exist_noncritical_q_pins_cells" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    timer.collect_non_critical_ffs_id();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    std::cout<<"Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<sequential_cells_id.size()<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/sequential_cells_id.size()<<std::endl;

    
    // clustering non critical cells
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();

             
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();

   
    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::cout<<"Clustering"<<std::endl;
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> noncritical_ff_cell_ids;
        
        // swap critical cells to best libcell
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(noncritical_q_pins_cells.find(cell_id) != noncritical_q_pins_cells.end()){
                noncritical_ff_cell_ids.insert(cell_id);
            }else{
                int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
                int current_lib_cell_id = cell.get_lib_cell_id();
                if(current_lib_cell_id == best_lib_cell_id){
                    continue;
                }
                netlist.swap_ff(cell_id,best_lib_cell_id);
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : noncritical_ff_cell_ids){
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
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }
        std::cout<<"Banking"<<std::endl;
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all() == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer
    for(int cell_id : netlist.get_sequential_cells_id()){
        timer.update_timing_and_cells_tns(cell_id);
    }

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}

bool CommandManager::iterate_clustering_exist_noncritical_q_pins_cells(){
    std::cout << "INIT iterate_clustering_exist_noncritical_q_pins_cells" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();

    int mid_bits_of_lib = ff_libcells_cost_manager.get_mid_bits_of_lib();        
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    std::vector<int> best_libcell_sorted_by_bits_limit_high;
    for(int bits : best_libcell_sorted_by_bits){
        if(bits <= mid_bits_of_lib){            
            best_libcell_sorted_by_bits_limit_high.push_back(bits);
        }
    }
    const int N = best_libcell_sorted_by_bits_limit_high.size();
        
    double clustering_percentage = 0.0;        

    for(int cell_id : netlist.get_sequential_cells_id()){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        if(cell.get_bits() > mid_bits_of_lib ){            
            int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());
            int current_lib_cell_id = cell.get_lib_cell_id();
            if(current_lib_cell_id == best_lib_cell_id){
                continue;
            }
            netlist.swap_ff(cell_id,best_lib_cell_id);
        }        
    }    

    //dynamic non critical q pins cells
    timer.collect_non_critical_ffs_id();
    std::unordered_set<int> dynamic_noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();

    bool first_time = true;
    while(first_time || clustering_percentage > 0.2){        
        first_time = false;
        // clustering non critical cells
        const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();                            
        for(const auto &it : clk_group_id_to_ff_cell_ids){            
            std::vector<std::vector<int>> clustering_res; 

            // remove critical q pins cells        
            std::cout<<"remove critical q pins cells"<<std::endl;    
            std::unordered_set<int> ff_cell_ids;
            for(int cell_id : it.second){
                if(dynamic_noncritical_q_pins_cells.count(cell_id)){
                    ff_cell_ids.insert(cell_id);
                }else{
                    const circuit::Cell &cell = netlist.get_cell(cell_id);
                    int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());
                    int current_lib_cell_id = cell.get_lib_cell_id();
                    if(current_lib_cell_id == best_lib_cell_id){
                        continue;
                    }
                    netlist.swap_ff(cell_id,best_lib_cell_id);
                }
            }

            // clustering
            std::unordered_map<int,int> bits_sum;
            std::unordered_map<int,std::vector<int>> bits_cell_ids;            
            std::cout<<"Clustering"<<std::endl;
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
                    if(best_libcell_sorted_by_bits_limit_high[idx] > sum){
                        idx++;
                    }else{
                        int num = sum / best_libcell_sorted_by_bits_limit_high[idx];
                        sum -= num * best_libcell_sorted_by_bits_limit_high[idx];
                        bits_clustering_map[bits][idx]+=num;
                    }

                }
            }
                
            for(const auto &it : bits_clustering_map){
                int bits = it.first;
                const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
                const std::vector<int> &matching_res = it.second;
                const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits_limit_high);            
                clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
            }
            std::cout<<"Banking"<<std::endl;
            // banking        
            for(const auto &cluster : clustering_res){ 
                netlist.cluster_cells(cluster);
            }            

            // legalizer replacement and remove
            //std::cout<<"Legalizer"<<std::endl;
            //for(const auto &cluster : clustering_res){
            //    int parent_id = cluster[0];
            //    legalizer.replacement_cell(parent_id);
            //    for(int i=1;i<static_cast<int>(cluster.size());i++){        
            //        legalizer.remove_cell(cluster[i]);        
            //    }
            //}    
        }

        // legalizer tetris    
        //legalizer.tetris();
        std::cout<<"Legalizer"<<std::endl;
        if(!legalizer.replace_all()){
            std::cout<<"Legal fail"<<std::endl;
            solution_manager.switch_to_best_solution();
            return false;
        }

        std::cout<<"timer"<<std::endl;
        // timer
        timer.update_timing_and_cells_tns();

        //for(int cell_id : netlist.get_sequential_cells_id()){
        //    timer.update_timing_and_cells_tns(cell_id);
        //}

        // utilization
        std::cout<<"Utilization"<<std::endl;
        utilization_calculator.update_bins_utilization();

        // cost
        // cost update
        std::cout<<"Cost"<<std::endl;
        cost_calculator.calculate_cost();

        // compare with best solution 

        double current_cost = cost_calculator.get_cost();
        double best_cost = solution_manager.get_best_solution().get_cost();
        std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
        if(current_cost < best_cost){
            std::cout<<"Keep best solution"<<std::endl;
            std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
            solution_manager.keep_best_solution();
            return true;
        }else{
            std::cout<<"rollback to best solution and update noncritical_q_pins_cells"<<std::endl;
            // update noncritical_q_pins_cells
            timer.collect_non_critical_ffs_id();
            const std::unordered_set<int> &new_noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();

            for(auto it = dynamic_noncritical_q_pins_cells.begin(); it != dynamic_noncritical_q_pins_cells.end();){
                // not in new noncritical_q_pins_cells: means banking will make it critical
                if(new_noncritical_q_pins_cells.find(*it) == new_noncritical_q_pins_cells.end()){
                    it = dynamic_noncritical_q_pins_cells.erase(it);
                }else{
                    it++;
                }
            }
            solution_manager.switch_to_best_solution();            
        }                
        clustering_percentage = static_cast<double>(dynamic_noncritical_q_pins_cells.size())/netlist.get_sequential_cells_id().size();
        std::cout<<"Next clustering percentage:"<<clustering_percentage<<std::endl;
    }
    return false;
}

bool CommandManager::clustering_all_noncritical_q_pins_cells(){
    std::cout << "INIT clustering_all_noncritical_q_pins_cells" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_all_non_critical_q_pin_ffs_id();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    std::cout<<"Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<sequential_cells_id.size()<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/sequential_cells_id.size()<<std::endl;
    return true;
}


bool CommandManager::clustering_exist_noncritical_q_pins_cells_and_legal_by_timing(){
    std::cout << "INIT clustering_exist_noncritical_q_pins_cells_and_legal_by_timing" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    timer.collect_non_critical_ffs_id();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    std::cout<<"Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<sequential_cells_id.size()<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/sequential_cells_id.size()<<std::endl;

    
    // clustering non critical cells
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();

             
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();

   
    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> noncritical_ff_cell_ids;
        
        // swap critical cells to best libcell
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(noncritical_q_pins_cells.find(cell_id) != noncritical_q_pins_cells.end()){
                noncritical_ff_cell_ids.insert(cell_id);
            }else{
                int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
                int current_lib_cell_id = cell.get_lib_cell_id();
                if(current_lib_cell_id == best_lib_cell_id){
                    continue;
                }
                netlist.swap_ff(cell_id,best_lib_cell_id);
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : noncritical_ff_cell_ids){
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
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }        
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    // update timer first
    timer.update_timing_and_cells_tns();

    const std::vector<int> &legal_order_cells_id = timer.get_timing_ranking_legalize_order_ffs_id();

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all_by_timing_order(legal_order_cells_id) == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer
    timer.update_timing_and_cells_tns();

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}


bool CommandManager::extremely_fast_clustering_and_legal_by_timing(){
    std::cout << "INIT extremely_fast_clustering_and_legal_by_timing" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    

    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();


    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();   

    int not_in_clk_group = 0;
    for(auto &cell : netlist.get_mutable_cells()){
        int cell_id = cell.get_id();
        if(netlist.get_clk_group_id(cell_id) == -1){            
            not_in_clk_group++;
            int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
            int current_lib_cell_id = cell.get_lib_cell_id();
            if(best_lib_cell_id == -1 || current_lib_cell_id == best_lib_cell_id){
                continue;
            }
            netlist.swap_ff(cell_id,best_lib_cell_id);
        }
    }
    std::cout<<"Not in clk group:"<<not_in_clk_group<<std::endl;


    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        const std::unordered_set<int> &ff_cell_ids = it.second;

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

        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }        
    }

    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing_and_cells_tns();
    const std::vector<int> &legal_order_cells_id = timer.get_timing_ranking_legalize_order_ffs_id();

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_all_by_timing_order(legal_order_cells_id) == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer    
    timer.update_timing_and_cells_tns();

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

}


bool CommandManager::dry_banking(){
    std::cout << "INIT dry_banking" << std::endl;    
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();        
    timer::Timer &timer = timer::Timer::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    // clustering non critical cells
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();             
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();
    timer.collect_non_critical_ffs_id();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();    
    int all_ffs_size = netlist.get_sequential_cells_id().size();
    std::cout<<"Exist Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<all_ffs_size<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/all_ffs_size<<std::endl;

    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> noncritical_ff_cell_ids;
        
        // swap critical cells to best libcell
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(noncritical_q_pins_cells.find(cell_id) != noncritical_q_pins_cells.end()){
                noncritical_ff_cell_ids.insert(cell_id);
            }else{
                int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
                int current_lib_cell_id = cell.get_lib_cell_id();
                if(current_lib_cell_id == best_lib_cell_id){
                    continue;
                }
                netlist.swap_ff(cell_id,best_lib_cell_id);
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : noncritical_ff_cell_ids){
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
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }        
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    std::cout<<"Clustering done"<<std::endl;    

    timer.update_timing_and_cells_tns();
    std::cout<<"timer done"<<std::endl;

    // cost    
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    cost_calculator.calculate_cost();
    std::cout<<"cost done"<<std::endl;
    
    double current_cost = cost_calculator.get_cost();

    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    double best_cost = solution_manager.get_best_solution().get_cost();
            
    std::cout<<"Dry run banking:"<<current_cost<<" init cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Let's use banking solution"<<std::endl;
        return true;
    }else{        
        solution_manager.switch_to_best_solution();
        std::cout<<"Rollback banking solution, timing case :("<<std::endl;
        return false;
    }

}

bool CommandManager::legal_by_timing(){    
    timer::Timer &timer = timer::Timer::get_instance();    
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();
    
    // legalizer
    const std::vector<int> &legal_order_cells_id = timer.get_timing_ranking_legalize_order_ffs_id();
    if( legalizer.replace_all_by_timing_order(legal_order_cells_id) == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer    
    timer.update_timing_and_cells_tns();

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"After Legal: New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    solution_manager.keep_best_solution();
    return true;
}



bool CommandManager::final_version_dry_banking(){
    std::cout << "INIT final_version_dry_banking" << std::endl;    
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();        
    timer::Timer &timer = timer::Timer::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    // clustering non critical cells
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();     
    const std::vector<int> &best_libcell_sorted_by_bits =  ff_libcells_cost_manager.get_best_libcell_sorted_by_bits();
    const int N = best_libcell_sorted_by_bits.size();
    timer.collect_non_critical_ffs_id();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_exist_non_critical_q_pin_ffs_id();    
    int all_ffs_size = netlist.get_sequential_cells_id().size();
    std::cout<<"Exist Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<all_ffs_size<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/all_ffs_size<<std::endl;

    for(const auto &it : clk_group_id_to_ff_cell_ids){        
        std::vector<std::vector<int>> clustering_res; 

        const std::unordered_set<int> &ff_cell_ids = it.second;        
        std::unordered_set<int> noncritical_ff_cell_ids;
        
        // swap critical cells to best libcell
        for(int cell_id : ff_cell_ids){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            if(noncritical_q_pins_cells.find(cell_id) != noncritical_q_pins_cells.end()){
                noncritical_ff_cell_ids.insert(cell_id);
            }else{
                int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(cell.get_bits());            
                int current_lib_cell_id = cell.get_lib_cell_id();
                if(current_lib_cell_id == best_lib_cell_id){
                    continue;
                }
                netlist.swap_ff(cell_id,best_lib_cell_id);
            }
        }

        std::unordered_map<int,int> bits_sum;
        std::unordered_map<int,std::vector<int>> bits_cell_ids;        
        for(int cell_id : noncritical_ff_cell_ids){
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
               
        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            const std::vector<std::vector<int>> &matching_clusters = divide_into_matching_cluster_by_y_order(cell_ids,bits,matching_res,best_libcell_sorted_by_bits);            
            clustering_res.insert(clustering_res.end(),matching_clusters.begin(),matching_clusters.end());
        }        
        // banking        
        for(const auto &cluster : clustering_res){ 
            netlist.cluster_cells(cluster);
        }                
    }

    std::cout<<"Clustering done"<<std::endl;    

    timer.update_timing_and_cells_tns();
    std::cout<<"timer done"<<std::endl;

    // cost    
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    cost_calculator.calculate_cost();
    std::cout<<"cost done"<<std::endl;
    
    double current_cost = cost_calculator.get_cost();

    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    double best_cost = solution_manager.get_best_solution().get_cost();
            
    std::cout<<"Dry run banking:"<<current_cost<<" init cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Let's use banking solution"<<std::endl;
        return true;
    }else{        
        solution_manager.switch_to_best_solution();
        std::cout<<"Rollback banking solution, timing case :("<<std::endl;
        return false;
    }

}



}