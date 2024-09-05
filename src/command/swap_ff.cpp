#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../estimator/lib_cell_evaluator.h"
#include "../runtime/runtime.h"
#include "../legalizer/legalizer.h"
#include "../legalizer/utilization.h"
namespace command{

void CommandManager::swap_ff(){
    std::cout << "INIT swap_ff" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();  
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();

    const std::unordered_map<int,int> &best_libcell_bits = ff_libcells_cost_manager.get_best_libcell_bits();

    for(int cell_id : sequential_cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        int bits = cell.get_bits();
        int lib_cell_id = cell.get_lib_cell_id(); 
        int best_lib_cell_id = best_libcell_bits.at(bits);      
        if(lib_cell_id == best_lib_cell_id){
            continue;
        }        
        netlist.swap_ff(cell_id,best_lib_cell_id);
    }

    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();    
    if( legalizer.replace_all() == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return;
    }
    
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing_and_cells_tns();
    //for(int cell_id : netlist.get_sequential_cells_id()){
    //    timer.update_timing_and_cells_tns(cell_id);
    //}

    utilization_calculator.update_bins_utilization();

    cost_calculator.calculate_cost();
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

    std::cout<<"FINISH swap_ff"<<std::endl;
    runtime.get_runtime();
}

void CommandManager::swap_ff_and_legal_by_timing(){
    std::cout << "INIT swap_ff_and_legal_by_timing" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    const estimator::FFLibcellCostManager &ff_libcells_cost_manager = estimator::FFLibcellCostManager::get_instance();
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();  
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();

    const std::unordered_map<int,int> &best_libcell_bits = ff_libcells_cost_manager.get_best_libcell_bits();

    for(int cell_id : sequential_cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        int bits = cell.get_bits();
        int lib_cell_id = cell.get_lib_cell_id(); 
        int best_lib_cell_id = best_libcell_bits.at(bits);      
        if(lib_cell_id == best_lib_cell_id){
            continue;
        }        
        netlist.swap_ff(cell_id,best_lib_cell_id);
    }

    std::cout<<"FINISH swap_ff"<<std::endl;

    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing_and_cells_tns();
    std::cout<<"First update timing finish"<<std::endl;
    const std::vector<int> &legal_order_cells_id = timer.get_timing_ranking_legalize_order_ffs_id();
    std::cout<<"get legal_order_cells_id"<<std::endl;

    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();    
    if( legalizer.replace_all_by_timing_order(legal_order_cells_id) == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return;
    }
    
    timer.update_timing_and_cells_tns();
    std::cout<<"Second update timing finish"<<std::endl;

    utilization_calculator.update_bins_utilization();

    cost_calculator.calculate_cost();
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

    std::cout<<"FINISH swap_ff"<<std::endl;
    runtime.get_runtime();
}



}