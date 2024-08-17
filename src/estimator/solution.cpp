#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "solution.h"
#include <iostream>
#include "../timer/timer.h"
#include "../legalizer/legalizer.h"
#include "../estimator/cost.h"
namespace estimator {    
void SolutionManager::keep_solution(int which_solution){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const timer::Timer &timer = timer::Timer::get_instance();
    const legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    const CostCalculator &cost_calculator = CostCalculator::get_instance();
    double cost = cost_calculator.get_cost();
    if(which_solution == 0){
        init_solution.update(cost,netlist.get_cells(),netlist.get_pins(),netlist.get_sequential_cells_id(),timer.get_timing_nodes(),legalizer.get_cell_id_to_site_id_map());
    }else if(which_solution == 1){
        current_solution.update(cost,netlist.get_cells(),netlist.get_pins(),netlist.get_sequential_cells_id(),timer.get_timing_nodes(),legalizer.get_cell_id_to_site_id_map());
    }else{
        best_solution.update(cost,netlist.get_cells(),netlist.get_pins(),netlist.get_sequential_cells_id(),timer.get_timing_nodes(),legalizer.get_cell_id_to_site_id_map());
    }
}

void SolutionManager::print_cost(const Solution &solution) const{
    std::cout << "COST:: " << solution.get_cost() << std::endl;
}

void SolutionManager::switch_to_other_solution(const Solution &solution){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    timer::Timer &timer = timer::Timer::get_instance();
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    CostCalculator &cost_calculator = CostCalculator::get_instance();
    netlist.switch_to_other_solution(solution.get_cells(),solution.get_pins(),solution.get_sequential_cells_id());
    legalizer.switch_to_other_solution(solution.get_cell_id_to_site_id_map());    
    timer.switch_to_other_solution(solution.get_timing_nodes());    
    cost_calculator.calculate_cost();
    double new_cost = cost_calculator.get_cost();
    // check
    if(new_cost != solution.get_cost()){
        std::cout<<"SOLUTION ERROR:: switch_to_other_solution:: cost is not correct"<<std::endl;
    }
}

}