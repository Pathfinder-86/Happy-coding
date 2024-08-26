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

void SolutionManager::update_best_solution_by_cells_id(const std::vector<int> &cells_id,double cost){
    // update cost
    this->best_solution.update_cost(cost);

    // update NETLIST and legal
    this->best_solution.update_cells_by_id(cells_id);
    
    // skip timer
}

void Solution::update_cells_by_id(const std::vector<int> &cells_id){
    // Cluster keep first and remove the rest
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    for(int cell_id : cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        update_single_cell(cell);
    }

    for(int i=1;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        remove_sequential_cell(cell_id);
        remove_cell_from_clk_group(cell_id);
        // legal
        remove_cell_id_to_sites_id(cell_id);
    }    

    const legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    const std::vector<int> &site_ids = legalizer.get_cell_site_ids(cells_id[0]);
    add_cell_id_to_site_id(cells_id[0],site_ids);
}

void SolutionManager::rollack_best_solution_by_cells_id(const std::vector<int> &cells_id){
    // ROLLBACK solution -> netlist
    this->best_solution.rollback_cells_by_id(cells_id);
}

void Solution::rollback_cells_by_id(const std::vector<int> &cells_id){
    // NETLIST
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int cid : cells_id){
        const circuit::Cell &cell = get_cell(cid);
        netlist.update_cell(cell);
        netlist.add_sequential_cell(cid);
        int clk_group_id = get_clk_group_id(cid);
        if(clk_group_id != -1){
            netlist.add_cell_to_clk_group(cid,clk_group_id);
        }
    }

    // LEGAL
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    int first_cell_id = cells_id[0];
    legalizer.replacement_cell(first_cell_id);
    for(int cid : cells_id){
        legalizer.update_cell_to_site(cid,get_cell_site_ids(cid));
    }
}

}