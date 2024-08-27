#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../estimator/lib_cell_evaluator.h"
#include "../runtime/runtime.h"
namespace command{

void CommandManager::swap_ff(){
    std::cout << "INIT swap_ff" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
    for(int cid : sequential_cells_id){
        const circuit::Cell &cell = netlist.get_cell(cid);
        int bits = cell.get_bits();
        int lib_cell_id = cell.get_lib_cell_id();
        int best_lib_cell_id = ff_libcells_cost_manager.get_best_libcell_for_bit(bits);
        if(lib_cell_id == best_lib_cell_id){
            continue;
        }
        if(ff_libcells_cost_manager.get_lib_cell_cost(lib_cell_id) <= ff_libcells_cost_manager.get_lib_cell_cost(best_lib_cell_id)){
            continue;
        }
        // swap
        netlist.swap_ff(cid,best_lib_cell_id);
    }

    cost_calculator.calculate_cost();
    double new_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<" New cost:"<<new_cost<<" Best cost:"<<best_cost<<std::endl;            
    if (new_cost < best_cost) {
        solution_manager.keep_best_solution();
        std::cout<<"SWAP_FF works!!! New cost is better than best cost"<<best_cost<<std::endl;
    }else{
        solution_manager.switch_to_best_solution();
        std::cout<<"SWAP_FF like a shit! New cost is worse than best cost rollback"<<best_cost<<std::endl;
    }
    runtime.get_runtime();
}


}