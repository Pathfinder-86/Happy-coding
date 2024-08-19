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
        const circuit::Cell& cell = cells.at(cell_id);
        const std::string &cell_name = netlist.get_cell_name(cell_id);
        double cell_timing_cost = 0.0;
        double cell_power_cost = 0.0;
        double cell_area_cost = 0.0;
        int bits = cell.get_bits();
        if(!cell.is_clustered()){
            double slack = cell.get_slack();
            if(slack < 0){
                cell_timing_cost = timing_factor * std::abs(slack);
            }            
            cell_power_cost = power_factor * cell.get_power();
            cell_area_cost = area_factor * cell.get_area();
            //std::cout<<"COSTCAL:: Cell:"<<cell_name<<" slack:"<<slack<<" timing_cost:"<<cell_timing_cost<<" power_cost:"<<cell_power_cost<<" area_cost:"<<cell_area_cost<<std::endl;
            timing_cost += cell_timing_cost;
            area_cost += cell_area_cost;
            power_cost += cell_power_cost;
        }        
        sequential_cells_cost.push_back( CellCost(cell_id,cell_timing_cost,cell_power_cost,cell_area_cost) );
        // TODO:
        // divide by bits since 2bits ff cost is definitely higher than 1 bit ff
        // but the truth is that 2bits ff is more efficient than 2 1bit ff considering the avg
        // cost adjusted by bits
        cell_timing_cost /= bits;
        cell_power_cost /= bits;
        cell_area_cost /= bits;
        adjusted_sequential_cells_cost.push_back( CellCost(cell_id,cell_timing_cost,cell_power_cost,cell_area_cost) );
    }
    // TODO:  add utilization cost
    const legalizer::UtilizationCalculator& utilization = legalizer::UtilizationCalculator::get_instance();
    int overflow_bins = utilization.get_overflow_bins();
    utilization_cost = utilization_factor * overflow_bins;    
    total_cost = area_cost + power_cost + timing_cost + utilization_cost;
    std::cout<<"COSTCAL:: INIT"<<std::endl;
    std::cout << "Total Cost: " << total_cost << std::endl;
    std::cout << "Area Cost: " << area_cost << std::endl;
    std::cout << "Power Cost: " << power_cost << std::endl;
    std::cout << "Timing Cost: " << timing_cost << std::endl;
    std::cout << "Utilization Cost: " << utilization_cost << std::endl;
    std::cout<<"COSTCAL:: END"<<std::endl;
}

}