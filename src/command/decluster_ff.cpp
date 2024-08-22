#include "../circuit/netlist.h"
#include <unordered_set>
#include "command_manager.h"
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../runtime/runtime.h"
#include "../legalizer/legalizer.h"
namespace command{

void CommandManager::test_decluster_ff() {    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const runtime::RuntimeManager& runtime = runtime::RuntimeManager::get_instance();        
    estimator::CostCalculator& cost_calculator = estimator::CostCalculator::get_instance();
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    std::cout<<"COMMAND:: test_decluster_ff START"<<std::endl;           
    runtime.get_runtime();    


    const std::vector<estimator::CellCost>& sequential_cells_cost = cost_calculator.get_sequential_cells_cost();
    if(sequential_cells_cost.size() < 1){
        std::cout<<"COMMAND test_decluster_ff::Not enough FF cells to decluster"<<std::endl;
        return;
    }
    int first_cell_id = sequential_cells_cost[0].get_id();
    // MSG START
    double first_cell_cost = sequential_cells_cost[0].get_total_cost();
    std::cout<<"COMMAND:: test_decluster_ff:: First cell:"<<first_cell_id<<" cost:"<<first_cell_cost<<std::endl;
    // MSG END

    int decluster_result = netlist.decluster_cells(first_cell_id);
    if(decluster_result == 0){
        std::cout<<"COMMAND:: test_decluster_ff:: Decluster success"<<std::endl;
    }else if(decluster_result != 1){ 
        // only legal fail will change circuit       
        // don't need to rollback solution
        return;
    }
    else{
        std::cout<<"COMMAND:: test_decluster_ff:: Decluster due to legal fail"<<std::endl;
        // solution rollback
        solution_manager.switch_to_other_solution(solution_manager.get_current_solution());
        return;
    }

    // update cost
    cost_calculator.calculate_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();    
    double new_cost = cost_calculator.get_cost();
    std::cout<<"COMMAND:: test cluster_ff:: New cost:"<<new_cost<<" best cost:"<<best_cost<<std::endl;    
    solution_manager.keep_best_solution();
    solution_manager.keep_current_solution();                
    std::cout<<"COMMAND:: test cluster_ff END"<<std::endl;       
}


}