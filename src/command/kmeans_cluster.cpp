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
#include <random>
#include <limits> 
#include <cmath>
#include <unordered_map>
namespace command{
void CommandManager::kmeans_cluster(){
    std::cout << "INIT Kmeans_cluster" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    int mid_bits_of_lib = ff_libcells_cost_manager.get_mid_bits_of_lib();


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
        if(ff_cell_ids.size() < 2){
            std::cout<<"Not enough FF cells to cluster"<<std::endl;
            break;
        }

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
            int temp = sum;
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
            //std::cout<<"bits: "<<bits<<" sum:"<<temp<<std::endl;
            //std::cout<<"clustering result: "<<std::endl;
            //for(int i=0;i<N;i++){
            //    std::cout<<best_libcell_sorted_by_bits[i]<<": "<<bits_clustering_map[bits][i]<<std::endl;
            //}
        }

        std::vector<std::vector<int>> clustering_res;

        for(const auto &it : bits_clustering_map){
            int bits = it.first;
            const std::vector<int> &cell_ids = bits_cell_ids.at(bits);
            const std::vector<int> &matching_res = it.second;
            int idx = 0;
            for(int i=0;i<matching_res.size();i++){
                int num = matching_res[i];
                if(num == 0){
                    continue;
                }
                int cluster_bit = best_libcell_sorted_by_bits[i];
                int ff_cells_each_cluster = cluster_bit / bits;
                std::vector<int> cluster;   
                for(int j=0;j<num;j++){
                    for(int k=0;k<ff_cells_each_cluster;k++){
                        int cell_id = cell_ids[ idx + j * ff_cells_each_cluster+k];
                        cluster.push_back(cell_id);
                    }
                    clustering_res.push_back(cluster);
                    cluster.clear();
                    //DEBUG
                    //std::cout<<"Cluster "<<cluster_bit<<": ";
                    //for(int cell_id : cluster){
                    //    const circuit::Cell &cell = netlist.get_cell(cell_id);
                    //    std::cout<<"id: "<<cell_id<<" bits:"<<cell.get_bits()<<" ";
                    //}
                    //std::cout<<std::endl;
                }                
                idx += num * ff_cells_each_cluster;
            }
        }

        estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
        int iteration = 0;
        for(const auto &cluster : clustering_res){
            std::cout<<"Iteration "<<iteration++<<std::endl; 
            for(int cell_id : cluster){
                const circuit::Cell &cell = netlist.get_cell(cell_id);
                std::cout<<"id: "<<cell_id<<" bits:"<<cell.get_bits()<<" ";
            }
            int cluster_res = netlist.cluster_cells(cluster);

            if(cluster_res == 0){
                std::cout<<"Cluster success"<<std::endl;
            }else if(cluster_res == -1){
                std::cout<<"Cluster fail due to legal rollback"<<std::endl;
                solution_manager.switch_to_best_solution();
                continue;
            }else{
                std::cout<<"Cluster fail"<<std::endl;
                continue;
            }

            cost_calculator.calculate_cost();
            double new_cost = cost_calculator.get_cost();
            double best_cost = solution_manager.get_best_solution().get_cost();

            std::cout<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;            
            if (new_cost < best_cost) {
                solution_manager.keep_best_solution();
                std::cout<<"New cost is better than best cost"<<best_cost<<std::endl;
            }else{
                solution_manager.switch_to_best_solution();
                std::cout<<"New cost is worse than best cost rollback (timing factor)"<<best_cost<<std::endl;
            }
        }
        solution_manager.keep_current_solution();

        // kmeans
        // decide K
    }
}

int calculate_distance(const std::pair<int, int>& node1, const std::pair<int, int>& node2) {
    return std::abs(node1.first - node2.first) + std::abs(node1.second - node2.second);
}

void k_means(const std::vector<std::pair<int, int>> &nodes_location, int K){    
    // Initialize centroids randomly
    std::vector<std::pair<int, int>> centroids;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, nodes_location.size() - 1);
    for (int i = 0; i < K; i++) {
        int index = dis(gen);
        centroids.push_back(nodes_location[index]);
    }

    // Assign nodes to clusters
    std::vector<std::vector<std::pair<int, int>>> clusters(K);
    for (const auto& node : nodes_location) {
        int min_distance = std::numeric_limits<int>::max();
        int closest_centroid = 0;
        
        for (int i = 0; i < K; i++) {
            int distance = calculate_distance(node, centroids[i]);
            if (distance < min_distance) {
                min_distance = distance;
                closest_centroid = i;
            }
        }
        clusters[closest_centroid].push_back(node);
    }

    // Update centroids
    bool centroids_updated = true;
    while (centroids_updated) {
        centroids_updated = false;
        for (int i = 0; i < K; i++) {
            if (clusters[i].empty()) {
                continue;
            }
            std::pair<int, int> sum = std::make_pair(0, 0);
            for (const auto& node : clusters[i]) {
                sum.first += node.first;
                sum.second += node.second;
            }
            std::pair<int, int> new_centroid = std::make_pair(sum.first / clusters[i].size(), sum.second / clusters[i].size());
            if (new_centroid != centroids[i]) {
                centroids[i] = new_centroid;
                centroids_updated = true;
            }
        }

        // Reassign nodes to clusters
        clusters.clear();
        clusters.resize(K);
        for (const auto& node : nodes_location) {
            double min_distance = std::numeric_limits<double>::max();
            int closest_centroid = 0;
            for (int i = 0; i < K; i++) {
                double distance = calculate_distance(node, centroids[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid = i;
                }
            }
            clusters[closest_centroid].push_back(node);
        }
    }

}



}