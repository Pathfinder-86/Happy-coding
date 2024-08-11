#include "cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../design/design.h"
#include "../legalizer/utilization.h"
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
        //int id = cell.get_id();
        const std::string &cell_name = netlist.get_cell_name(cell_id);
        const circuit::Cell& cell = cells.at(cell_id);
        area_cost += area_factor * cell.get_area();
        power_cost += power_factor * cell.get_power();
        timing_cost += timing_factor * cell.get_slack();
        std::cout<<"cell: "<<cell_name<<std::endl;
        std::cout<<"area: "<<cell.get_area()<<" "<<cell.get_area() * area_factor<<std::endl;
        std::cout<<"power: "<<cell.get_power()<<" "<<cell.get_power() * power_factor<<std::endl;
        std::cout<<"slack: "<<cell.get_slack()<<" "<<cell.get_slack() * timing_factor<<std::endl;
    }
    // TODO:  add utilization cost
    const legalizer::UtilizationCalculator& utilization = legalizer::UtilizationCalculator::get_instance();
    int overflow_bins = utilization.get_overflow_bins();
    utilization_cost = utilization_factor * overflow_bins;

    total_cost = area_cost + power_cost + timing_cost + utilization_cost;    
}
}