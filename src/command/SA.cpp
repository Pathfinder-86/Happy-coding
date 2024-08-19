#include "../circuit/netlist.h"
#include <unordered_set>
#include "command_manager.h"
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../runtime/runtime.h"
#include "../legalizer/legalizer.h"
#include <cmath>
#include "../config/config_manager.h"
namespace command{

void CommandManager::SA() {    
    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const runtime::RuntimeManager& runtime = runtime::RuntimeManager::get_instance();        
    estimator::CostCalculator& cost_calculator = estimator::CostCalculator::get_instance();
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    std::cout<<"COMMAND:: test SA cluster_ff START"<<std::endl;
    const config::ConfigManager& config_manager = config::ConfigManager::get_instance();
    runtime.get_runtime();

    // SA
    const double initial_temperature = 100.0;
    const double cooling_rate = std::get<double>(config_manager.get_config_value("cooling_rate"));
    const int max_iterations = std::get<int>(config_manager.get_config_value("max_iterations"));

    double current_temperature = initial_temperature;

    double clustering_rate = std::get<double>(config_manager.get_config_value("clustering_rate"));
    double declustering_rate = std::get<double>(config_manager.get_config_value("declustering_rate"));

    for (int iteration = 0; iteration < max_iterations; iteration++) {
        // clustering or declustering
        std::cout<<"SA:: Iteration:"<<iteration<<" Start"<<std::endl;
        const std::unordered_map<int, std::unordered_set<int>> &clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();
        int which_clk_group_idx = rand() % clk_group_id_to_ff_cell_ids.size();
        int which_clk_group_id = std::next(clk_group_id_to_ff_cell_ids.begin(), which_clk_group_idx)->first;
        const std::unordered_set<int> &ff_cell_ids = std::next(clk_group_id_to_ff_cell_ids.begin(), which_clk_group_idx)->second;

        if (rand() % 100 < clustering_rate * 100) {
            // clustering            
            if (ff_cell_ids.size() < 2) {
                std::cout << "Not enough FF cells to cluster" << std::endl;
                continue;
            }
            // random pick two cells_id for sequential cells
            int size = ff_cell_ids.size();
            int first_idx = rand() % size;
            int second_idx = rand() % size;
            while (first_idx == second_idx) {
                second_idx = rand() % size;
            }
            int first_cell_id = *std::next(ff_cell_ids.begin(), first_idx);
            int second_cell_id = *std::next(ff_cell_ids.begin(), second_idx);
            
            bool cluster_success = netlist.cluster_cells(first_cell_id, second_cell_id);
            if(!cluster_success){
                std::cout<<"Cluster fail"<<std::endl;
                continue;
            }else{
                std::cout<<"Cluster cell:"<<first_cell_id<<" and cell:"<<second_cell_id<<std::endl;
            }            
        } else {
            // declustering
            // random pick a idx in sequential cells
            if(ff_cell_ids.size() < 1){
                std::cout<<"No sequential cells to decluster"<<std::endl;
                continue;
            }
            int size = ff_cell_ids.size();
            int idx = rand() % size;
            int cell_id = *std::next(ff_cell_ids.begin(), idx);
            bool decluster_success = netlist.decluster_cells(cell_id);
            if(!decluster_success){
                std::cout<<"Decluster fail"<<std::endl;
                continue;
            }else{
                std::cout<<"Decluster cell:"<<cell_id<<std::endl;
            }            
        }    
        
        cost_calculator.calculate_cost();
        double new_cost = cost_calculator.get_cost();
        double current_cost = solution_manager.get_current_solution().get_cost();
        double best_cost = solution_manager.get_best_solution().get_cost();

        std::cout<<" Current cost:"<<current_cost<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;

        if (new_cost < current_cost) {
            std::cout<<"New cost is better than current cost"<<current_cost<<std::endl;
            solution_manager.keep_current_solution();
            if (new_cost < best_cost) {
                solution_manager.keep_best_solution();
                std::cout<<"New cost is better than best cost"<<best_cost<<std::endl;
            }
        } else {
            std::cout<<"New cost is not better than current cost"<<current_cost<<std::endl;
            double delta = new_cost - current_cost;
            double probability = 0.0;
            if (current_temperature != 0.0) {
                probability = std::exp(-delta / current_temperature);
            }            
            std::cout<<"Probability:"<<probability<<std::endl;
            if (rand() % 100 < probability * 100) {
                solution_manager.keep_current_solution();
                std::cout<<"Accept new cost even it is worse than current cost"<<std::endl;
            }else{
                // undo clustering or declustering
                solution_manager.switch_to_current_solution();
                std::cout<<"Undo clustering or declustering"<<std::endl;
            }
        }
        std::cout<<"SA:: Iteration:"<<iteration<<" END"<<std::endl;
        std::cout<<"SA:: Current temperature:"<<current_temperature<<std::endl;
        std::cout<<"--------------------------------------------"<<std::endl;
        current_temperature *= cooling_rate;
        runtime.get_runtime();
        if(runtime.is_timeout()){
            break;
        }
        if(current_temperature < 0.1){
            break;
        }
    }

    solution_manager.switch_to_best_solution();
    std::cout<<"COMMAND:: test SA END"<<std::endl;    
}
}