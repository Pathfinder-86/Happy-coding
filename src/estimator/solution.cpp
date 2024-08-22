#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "solution.h"
#include <iostream>
#include "../timer/timer.h"
#include "../legalizer/legalizer.h"
#include "../estimator/cost.h"
#include "../legalizer/utilization.h"
namespace estimator {    
void SolutionManager::keep_solution(int which_solution){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const timer::Timer &timer = timer::Timer::get_instance();
    const legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    const CostCalculator &cost_calculator = CostCalculator::get_instance();    
    if(which_solution == 0){
        init_solution.update(cost_calculator,netlist,timer,legalizer);
    }else if(which_solution == 1){
        current_solution.update(cost_calculator,netlist,timer,legalizer);
    }else{
        best_solution.update(cost_calculator,netlist,timer,legalizer);
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
    netlist.switch_to_other_solution(solution.get_cells(),solution.get_sequential_cells_id(),solution.get_clk_group_id_to_ff_cell_ids(),solution.get_ff_cell_id_to_clk_group_id());
    legalizer.switch_to_other_solution(solution.get_cell_id_to_site_id_map());    
    timer.switch_to_other_solution(solution.get_timing_nodes());
    legalizer::UtilizationCalculator &utilization = legalizer::UtilizationCalculator::get_instance();
    utilization.update_bins_utilization();
    cost_calculator.calculate_cost();
}

}