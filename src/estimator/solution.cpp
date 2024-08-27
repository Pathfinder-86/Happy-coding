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

void SolutionManager::update_best_solution_after_clustering(const std::vector<int> &cells_id,double cost){
    // update cost
    this->best_solution.update_cost(cost);

    // update NETLIST and legal
    this->best_solution.update_cells_by_id(cells_id);
    
    
}

void Solution::update_cells_by_id(const std::vector<int> &cells_id){
    // Netlist
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    // cells
    for(int cell_id : cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        update_single_cell(cell);
    }

    // sequential cells, clk groups
    for(int i=1;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        remove_sequential_cell(cell_id);
        remove_cell_from_clk_group(cell_id);                
    }

    // legalizer: cell_id_to_site_id_map
    update_legal_cell_site_mapping_after_clustering(cells_id);
}   

void Solution::update_legal_cell_site_mapping_after_clustering(const std::vector<int> &cells_id){
    const legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    for(int i=0;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        if(i==0){
            const std::vector<int> &site_ids = legalizer.get_cell_site_ids(cell_id);
            add_cell_id_to_site_id(cell_id,site_ids);
        }else{
            remove_cell_id_to_sites_id(cell_id);
        }
    }
}

void Solution::rollback_legal_cell_site_mapping_after_clustering(const std::vector<int> &cells_id){
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();    
    int first_cell_id = cells_id[0];
    legalizer.replacement_cell(first_cell_id);
    for(int cid : cells_id){
        legalizer.update_cell_to_site(cid,get_cell_site_ids(cid));
    }    
}

void SolutionManager::rollack_clustering_res_using_best_solution_skip_timer(const std::vector<int> &cells_id){
    // ROLLBACK solution -> netlist
    this->best_solution.rollback_cells_by_id(cells_id);
    // legalizer
    this->best_solution.rollback_legal_cell_site_mapping_after_clustering(cells_id);
}

void SolutionManager::rollack_clustering_res_using_best_solution(const std::vector<int> &cells_id){
    // ROLLBACK solution -> netlist
    this->best_solution.rollback_cells_by_id(cells_id);
    // timer    
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing_by_cells_id(cells_id);
    // legalizer
    this->best_solution.rollback_legal_cell_site_mapping_after_clustering(cells_id);
}

void Solution::rollback_cells_by_id(const std::vector<int> &cells_id){
    // NETLIST
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    design::Design &design = design::Design::get_instance();
    // CELL
    for(int cid : cells_id){
        // from best_solution
        const circuit::Cell &cell = get_cell(cid);

        // rollback
        netlist.update_cell(cell);
        netlist.add_sequential_cell(cid);
        int clk_group_id = get_clk_group_id(cid);
        if(clk_group_id != -1){
            netlist.add_cell_to_clk_group(cid,clk_group_id);
        }else{
            std::cout<<"ERROR: clk_group_id not found"<<std::endl;
        }
        
        // PIN
        int cell_x = cell.get_x();
        int cell_y = cell.get_y();
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        const std::vector<std::pair<double, double>> &input_pins_loc = lib_cell.get_input_pins_position();
        const std::vector<std::pair<double, double>> &output_pins_loc = lib_cell.get_output_pins_position();
        int bits = lib_cell.get_bits();
        const std::vector<int> &input_pins_id = cell.get_input_pins_id();
        const std::vector<int> &output_pins_id = cell.get_output_pins_id();
        // input and output
        for(int i=0;i<bits;i++){
            int input_pin_id = input_pins_id[i];
            int output_pin_id = output_pins_id[i];
            circuit::Pin &input_pin = netlist.get_mutable_pin(input_pin_id);
            circuit::Pin &output_pin = netlist.get_mutable_pin(output_pin_id);
            input_pin.set_cell_id(cid);
            output_pin.set_cell_id(cid);
            input_pin.set_offset_x(input_pins_loc[i].first);
            input_pin.set_offset_y(input_pins_loc[i].second);
            output_pin.set_offset_x(output_pins_loc[i].first);
            output_pin.set_offset_y(output_pins_loc[i].second);
            input_pin.set_x(cell_x + input_pins_loc[i].first);
            input_pin.set_y(cell_y + input_pins_loc[i].second);
            output_pin.set_x(cell_x + output_pins_loc[i].first);
            output_pin.set_y(cell_y + output_pins_loc[i].second);
        }
    }
}

}