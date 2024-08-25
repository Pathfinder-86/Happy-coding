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
    /*
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
            const circuits::Cell &cell = netlist.get_cell(cell_id);
            int bits = cell.get_bits();
            bits_sum[bits] += bits;
            bits_cell_ids[bits].push_back(cell_id);                        
        }
        
        std::vector<std::pair<int,int>> nodes_location;        
        int current_bits = 0;
        for(int i=0;i<cells_bits.size();i++){
            current_bits+=cells_bits[i].second;
            
        }
        k_means()

        // kmeans
        // decide K
    }
    */
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