#include "../circuit/netlist.h"
#include <unordered_set>
#include "command_manager.h"
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../runtime/runtime.h"
#include "../legalizer/legalizer.h"
namespace command{

void CommandManager::test_cluster_ff() {    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const runtime::RuntimeManager& runtime = runtime::RuntimeManager::get_instance();        
    estimator::CostCalculator& cost_calculator = estimator::CostCalculator::get_instance();
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    std::cout<<"COMMAND:: test cluster_ff START"<<std::endl;           
    runtime.get_runtime();    


    const std::vector<estimator::CellCost>& sequential_cells_cost = cost_calculator.get_sequential_cells_cost();
    if(sequential_cells_cost.size() < 2){
        std::cout<<"COMMAND test_cluster_ff::Not enough FF cells to cluster"<<std::endl;
        return;
    }
    int first_cell_id = sequential_cells_cost[0].get_id();
    int second_cell_id = sequential_cells_cost[1].get_id();
    // MSG START
    const std::string& first_cell_name = netlist.get_cell_name(first_cell_id);
    const std::string& second_cell_name = netlist.get_cell_name(second_cell_id);
    double first_cell_cost = sequential_cells_cost[0].get_total_cost();
    double second_cell_cost = sequential_cells_cost[1].get_total_cost();
    std::cout<<"COMMAND:: test cluster_ff:: First cell:"<<first_cell_name<<" cost:"<<first_cell_cost<<" Second cell:"<<second_cell_name<<" cost:"<<second_cell_cost<<std::endl;
    // MSG END

    netlist.cluster_cells(first_cell_id,second_cell_id);    
    // legalize
    legalizer::Legalizer& legalizer = legalizer::Legalizer::get_instance();
    if(legalizer.check_on_site()){
        std::cout<<"LEGAL: All cells are on site"<<std::endl;
    }else{
        std::cout<<"LEGAL: Some cells are not on site do legalization"<<std::endl;
        if( legalizer.legalize()){
            std::cout<<"LEGAL: legalization success"<<std::endl;
        }else{
            std::cout<<"LEGAL: legalization fail"<<std::endl;
        }
    }

    // update cost
    cost_calculator.calculate_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();    
    double new_cost = cost_calculator.get_cost();
    if(new_cost < best_cost){
        std::cout<<"COMMAND:: test cluster_ff:: New cost:"<<new_cost<<" is better than best cost:"<<best_cost<<std::endl;
        solution_manager.keep_best_solution();
        solution_manager.keep_current_solution();
    }else{
        std::cout<<"COMMAND:: test cluster_ff:: New cost:"<<new_cost<<" is not better than best cost:"<<best_cost<<std::endl;
        //netlist.uncluster_cells(first_cell_id,second_cell_id);
    }
    // update solution    
    std::cout<<"COMMAND:: test cluster_ff END"<<std::endl;   
    runtime.get_runtime();
}


void CommandManager::test_SA_cluster_ff() {    
    /*
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const runtime::RuntimeManager& runtime = runtime::RuntimeManager::get_instance();        
    estimator::CostCalculator& cost_calculator = estimator::CostCalculator::get_instance();
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    std::cout<<"COMMAND:: test SA cluster_ff START"<<std::endl;
    runtime.get_runtime();

    // SA
    const double initial_temperature = 100.0;
    const double cooling_rate = 0.95;
    const int max_iterations = 1000;

    double current_temperature = initial_temperature;
    estimator::CostCalculator& cost_calculator = estimator::CostCalculator::get_instance();
    double current_cost = cost_calculator.get_cost();
    double best_cost = current_cost;
    std::unordered_set<int> best_solution;

    for (int iteration = 0; iteration < max_iterations; iteration++) {
        std::unordered_set<int> new_solution = generate_new_solution();
        double new_cost = calculate_cost(new_solution);

        if (accept_new_solution(current_cost, new_cost, current_temperature)) {
            current_cost = new_cost;
            if (new_cost < best_cost) {
                best_cost = new_cost;
                best_solution = new_solution;
            }
        }

        current_temperature *= cooling_rate;
    }

    // Apply the best solution
    apply_solution(best_solution);
    */
}



}