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
    double first_cell_cost = sequential_cells_cost[0].get_total_cost();
    double second_cell_cost = sequential_cells_cost[1].get_total_cost();
    std::cout<<"COMMAND:: test cluster_ff:: First cell:"<<first_cell_id<<" cost:"<<first_cell_cost<<" Second cell:"<<second_cell_id<<" cost:"<<second_cell_cost<<std::endl;
    // MSG END
    std::vector<int> cells_id = {first_cell_id , second_cell_id};
    int cluster_res = netlist.cluster_cells(cells_id);
    if(cluster_res == 0){
        std::cout<<"COMMAND:: test cluster_ff:: Cluster success"<<std::endl;
    }else if(cluster_res != 1){ 
        // only legal fail will change circuit       
        // don't need to rollback solution
        std::cout<<"COMMAND:: test cluster_ff:: Cluster fail"<<std::endl;
        return;
    }
    else{
        std::cout<<"COMMAND:: test cluster_ff:: Cluster fail due to legal fail rollback"<<std::endl;
        // solution rollback
        solution_manager.switch_to_other_solution(solution_manager.get_current_solution());
        return;
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
    }    
    std::cout<<"COMMAND:: test cluster_ff END"<<std::endl;   
    runtime.get_runtime();
}


}