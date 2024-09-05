#include "command_manager.h"
#include <iostream>
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../legalizer/legalizer.h"
#include "../config/config_manager.h"
#include "../estimator/lib_cell_evaluator.h"
#include "../legalizer/utilization.h"
#include <algorithm>
#include <limits> 
#include <cmath>
#include <unordered_map>
#include "../runtime/runtime.h"
#include "../circuit/original_netlist.h"
#include <iomanip>
namespace command{

bool CommandManager::cell_move_recover_tns(){
    std::cout << "INIT cell_move_recover_tns" << std::endl;
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();    
    runtime::RuntimeManager &runtime = runtime::RuntimeManager::get_instance();
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();
    const std::unordered_set<int> &noncritical_q_pins_cells = timer.get_all_non_critical_q_pin_ffs_id();    
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    std::cout<<"All Non critical q pins cells:"<<noncritical_q_pins_cells.size()<<" Total sequential cells:"<<sequential_cells_id.size()<<" percentage:"<<static_cast<double>(noncritical_q_pins_cells.size())/sequential_cells_id.size()<<std::endl;

    // extract non critical q pins cells which tns > 0
    std::vector<std::pair<int,double>> sorted_tns_noncritical_q_pins_cells;
    for(int cell_id : noncritical_q_pins_cells){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        double tns = cell.get_tns();
        if(tns > 0){
            sorted_tns_noncritical_q_pins_cells.push_back(std::make_pair(cell_id,tns));
        }
    }

    int commit_size = sorted_tns_noncritical_q_pins_cells.size() * 0.1;
    std::cout<<"tns_noncritical_q_pins_cells size:"<<sorted_tns_noncritical_q_pins_cells.size()<<" Commit size:"<<commit_size<<std::endl;
    if(commit_size < 100){
        std::cout<<"Commit size is too small"<<std::endl;
        return true;
    } 

    std::sort(sorted_tns_noncritical_q_pins_cells.begin(),sorted_tns_noncritical_q_pins_cells.end(),[](const std::pair<int,double> &a,const std::pair<int,double> &b){
        return a.second > b.second;
    });

    std::unordered_set<int> moved_cells;
    for(const auto &it : sorted_tns_noncritical_q_pins_cells){
        int cell_id = it.first;
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        std::pair<int,int> new_cell_location = std::make_pair(0,0);
        int count = 0;
        for(int input_pin_id : cell.get_input_pins_id()){
            double slack = timer.get_slack(input_pin_id);
            if(slack >= 0){
                continue;
            }
            std::pair<int,int> fanin_location = timer.get_ff_input_pin_fanin_location(input_pin_id);
            const circuit::Pin &pin = netlist.get_pin(input_pin_id);
            new_cell_location.first += fanin_location.first;
            new_cell_location.first += pin.get_x();
            new_cell_location.second += fanin_location.second;
            new_cell_location.second += pin.get_y();
            count+=2;
        }
        if(count == 0){
            continue;
        }        
        new_cell_location.first /= count;
        new_cell_location.second /= count;         
        cell.move(new_cell_location.first,new_cell_location.second);
        moved_cells.insert(cell_id);
        commit_size--;
        if(commit_size <= 0){
            break;
        }
    }

    // legalizer
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    if( legalizer.replace_some(moved_cells) == false){
        // rollback
        std::cout<<"Legal fail"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }

    // timer
    timer.update_timing_and_cells_tns();

    // utilization
    utilization_calculator.update_bins_utilization();

    // cost
    // cost update
    cost_calculator.calculate_cost();

    // compare with best solution 

    double current_cost = cost_calculator.get_cost();
    double best_cost = solution_manager.get_best_solution().get_cost();
    std::cout<<"New cost:"<<current_cost<<" Best cost:"<<best_cost<<std::endl;
    if(current_cost < best_cost){
        std::cout<<"Keep best solution"<<std::endl;
        std::cout<<"COST/RUNTIME "<<current_cost<<" "<<runtime.get_runtime_seconds()<<std::endl;
        solution_manager.keep_best_solution();
        return true;
    }else{
        std::cout<<"rollback to best solution"<<std::endl;
        solution_manager.switch_to_best_solution();
        return false;
    }
}


}