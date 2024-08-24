#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuits/netlist.h"
#include "../legalizer/legalizer.h"
#include "../config/config_manager.h"
void CommandManager::kmeans_cluster(){
    std::cout << "kmeans_cluster" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    circuits::Netlist &netlist = circuits::Netlist::get_instance();
    const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = 
    int clk_group = 

}