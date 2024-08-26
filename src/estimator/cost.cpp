#include "cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../design/design.h"
#include "../legalizer/utilization.h"
#include "../design/libcell.h"
namespace estimator{

void CostCalculator::calculate_cost(){
    reset();    
    const design::Design& design = design::Design::get_instance();
    double timing_factor = design.get_timing_factor();
    double power_factor = design.get_power_factor();
    double area_factor = design.get_area_factor();
    double utilization_factor = design.get_utilization_factor();

    circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int cell_id : netlist.get_sequential_cells_id()){                 
        const circuit::Cell& cell = cells.at(cell_id);     
        double cell_timing_cost = 0.0; 
        double slack = cell.get_slack();
        if(slack < 0){
            cell_timing_cost = timing_factor * std::abs(slack);
        }            
        double cell_power_cost = power_factor * cell.get_power();
        double cell_area_cost = area_factor * cell.get_area();        
        timing_cost += cell_timing_cost;
        area_cost += cell_area_cost;
        power_cost += cell_power_cost;

        sequential_cells_cost.push_back( CellCost(cell_id,cell_timing_cost,cell_power_cost,cell_area_cost) );
    }
    // TODO:  add utilization cost
    const legalizer::UtilizationCalculator& utilization = legalizer::UtilizationCalculator::get_instance();
    int overflow_bins = utilization.get_overflow_bins();
    utilization_cost = utilization_factor * overflow_bins;
    std::cout<<"COSTCAL:: INIT"<<std::endl;
    std::cout << "Total Cost: " << get_cost() << std::endl;
    std::cout << "Area Cost: " << area_cost << std::endl;
    std::cout << "Power Cost: " << power_cost << std::endl;
    std::cout << "Timing Cost: " << timing_cost << std::endl;
    std::cout << "Utilization Cost: " << utilization_cost << std::endl;
    std::cout<<"COSTCAL:: END"<<std::endl;
}

void CostCalculator::calculate_cost_update_by_cells_id(const std::vector<int> &cells_id){  
    const design::Design& design = design::Design::get_instance();
    double timing_factor = design.get_timing_factor();
    double power_factor = design.get_power_factor();
    double area_factor = design.get_area_factor();
    double utilization_factor = design.get_utilization_factor();

    circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int i=0;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        double new_cell_area_cost = 0.0;
        double new_cell_power_cost = 0.0;
        if(i==0){            
            const circuit::Cell& cell = cells.at(cell_id);
            int lib_cell_id = cell.get_lib_cell_id();
            const design::LibCell& lib_cell = design.get_lib_cell(lib_cell_id);
            // SKIP TIMING COST
            new_cell_power_cost = power_factor * lib_cell.get_power();
            new_cell_area_cost = area_factor * lib_cell.get_area();      
        }      
        area_cost += (new_cell_area_cost - sequential_cells_cost[cell_id].get_area_cost());
        power_cost += (new_cell_power_cost - sequential_cells_cost[cell_id].get_power_cost());
        sequential_cells_cost[cell_id].set_area_cost(new_cell_area_cost);
        sequential_cells_cost[cell_id].set_power_cost(new_cell_power_cost);
    }
}

void CostCalculator::calculate_cost_rollback_by_cells_id(const std::vector<int> &cells_id){  
    const design::Design& design = design::Design::get_instance();
    double timing_factor = design.get_timing_factor();
    double power_factor = design.get_power_factor();
    double area_factor = design.get_area_factor();
    double utilization_factor = design.get_utilization_factor();

    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int cell_id : cells_id){                 
        const circuit::Cell& cell = cells.at(cell_id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell& lib_cell = design.get_lib_cell(lib_cell_id);
        // SKIP TIMING COST
        double new_cell_power_cost = power_factor * lib_cell.get_power();
        double new_cell_area_cost = area_factor * lib_cell.get_area();
        area_cost += (new_cell_area_cost - sequential_cells_cost[cell_id].get_area_cost());
        power_cost += (new_cell_power_cost - sequential_cells_cost[cell_id].get_power_cost());
        sequential_cells_cost[cell_id].set_area_cost(new_cell_area_cost);
        sequential_cells_cost[cell_id].set_power_cost(new_cell_power_cost);
    }
}

}