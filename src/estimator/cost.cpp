#include "cost.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../design/design.h"
#include "../legalizer/utilization.h"
#include "../design/libcell.h"
#include "lib_cell_evaluator.h"
#include <iomanip>
namespace estimator{

void CostCalculator::set_factors(){
    const design::Design& design = design::Design::get_instance();
    timing_factor = design.get_timing_factor();
    power_factor = design.get_power_factor();
    area_factor = design.get_area_factor();
    utilization_factor = design.get_utilization_factor();
}

void CostCalculator::calculate_cost(){
    reset();        
    const FFLibcellCostManager& ff_libcell_cost_manager = FFLibcellCostManager::get_instance();
    circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();    
    for(int i=0;i<cells.size();i++){            
        const circuit::Cell& cell = cells.at(i);
        double cell_timing_cost = 0.0;
        double cell_power_cost = 0.0;
        double cell_area_cost = 0.0;
        if(!cell.is_clustered()){                    
            cell_timing_cost = cell.get_tns() * timing_factor; 
            int lib_cell_id = cell.get_lib_cell_id();
            const FFLibCellCost& lib_cell_cost = ff_libcell_cost_manager.get_ff_libcell_cost(lib_cell_id);
            cell_power_cost = lib_cell_cost.get_power_cost();
            cell_area_cost = lib_cell_cost.get_area_cost();        
        }
        add_timing_cost(cell_timing_cost);
        add_power_cost(cell_power_cost);
        add_area_cost(cell_area_cost);
        sequential_cells_cost.push_back( CellCost(i,cell_timing_cost,cell_power_cost,cell_area_cost) );        
    }

    
    std::cout<<"COSTCAL:: INIT"<<std::endl;
    std::cout << "Total Cost: " << std::fixed << std::setprecision(5) << get_cost() << std::endl;
    std::cout << "Area Cost: " << get_area_cost() << std::endl;
    std::cout << "Power Cost: " << get_power_cost() << std::endl;
    std::cout << "Timing Cost: " << get_timing_cost() << std::endl;
    std::cout << "Utilization Cost: " << get_utilization_cost() << std::endl;
    std::cout<<"COSTCAL:: END"<<std::endl;
}

void CostCalculator::update_cells_cost_after_clustering_skip_timer(const std::vector<int> &cells_id){  
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int i=0;i<cells_id.size();i++){        
        int cell_id = cells_id[i];
        const circuit::Cell& cell = cells.at(cell_id);                     
        if(i==0){            
            update_cell_cost_by_libcell_id(cell_id,cell.get_lib_cell_id());
        }else{
            update_cell_cost_been_clustered(cell_id);
        }
    }
}

void CostCalculator::update_cells_cost_after_clustering(const std::vector<int> &cluster_cells_id, const std::vector<int> &timing_cells_id){
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int i=0;i<cluster_cells_id.size();i++){
        int cell_id = cluster_cells_id[i];
        const circuit::Cell& cell = cells.at(cell_id);
        if(i==0){
            update_cell_cost_by_libcell_id(cell_id,cell.get_lib_cell_id());            
        }else{            
            update_cell_cost_been_clustered(cell_id);
        }
    }    

    for(int cell_id : timing_cells_id){
        update_cell_cost_by_slack(cell_id,cells.at(cell_id).get_tns());
    }

}

void CostCalculator::update_cell_cost_been_clustered(int cell_id){
    add_timing_cost(-sequential_cells_cost[cell_id].get_timing_cost());
    add_power_cost(-sequential_cells_cost[cell_id].get_power_cost());
    add_area_cost(-sequential_cells_cost[cell_id].get_area_cost());        
    sequential_cells_cost[cell_id].set_zero();
}

void CostCalculator::update_cell_cost_by_slack(int cell_id,double tns){        
    double new_cell_timing_cost = tns * timing_factor;          
    double original_cell_timing_cost = sequential_cells_cost[cell_id].get_timing_cost();
    add_timing_cost(new_cell_timing_cost - original_cell_timing_cost);

    sequential_cells_cost[cell_id].set_timing_cost(new_cell_timing_cost);        
}

void CostCalculator::update_cell_cost_by_libcell_id(int cell_id,int lib_cell_id){
    const FFLibcellCostManager& ff_libcell_cost_manager = FFLibcellCostManager::get_instance();
    const FFLibCellCost& lib_cell_cost = ff_libcell_cost_manager.get_ff_libcell_cost(lib_cell_id);
    double new_cell_power_cost = lib_cell_cost.get_power_cost();
    double new_cell_area_cost = lib_cell_cost.get_area_cost();    
    double original_cell_power_cost = sequential_cells_cost[cell_id].get_power_cost();
    double original_cell_area_cost = sequential_cells_cost[cell_id].get_area_cost();    
    add_power_cost(new_cell_power_cost - original_cell_power_cost);
    add_area_cost(new_cell_area_cost - original_cell_area_cost);

    sequential_cells_cost[cell_id].set_area_cost(new_cell_area_cost);
    sequential_cells_cost[cell_id].set_power_cost(new_cell_power_cost);
}

double CostCalculator::get_utilization_cost() const{
    const design::Design& design = design::Design::get_instance();
    double utilization_factor = design.get_utilization_factor();
    const legalizer::UtilizationCalculator& utilization = legalizer::UtilizationCalculator::get_instance();    
    return utilization.get_overflow_bins_num() * utilization_factor;
}

void CostCalculator::rollack_timer_res(const std::vector<int> &cells_id){      
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();    
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(int cell_id : cells_id){
        const circuit::Cell& cell = cells.at(cell_id);        
        double new_cell_timing_cost = cell.get_tns() * timing_factor;    
        add_timing_cost(new_cell_timing_cost - sequential_cells_cost[cell_id].get_timing_cost());
        sequential_cells_cost[cell_id].set_timing_cost(new_cell_timing_cost);                          
    }
}

void CostCalculator::rollack_clustering_res(const std::vector<int> &cells_id){      
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();    
    const std::vector<circuit::Cell>& cells = netlist.get_cells();    
    const FFLibcellCostManager &ff_libcell_cost_manager = FFLibcellCostManager::get_instance();
    for(int i=0;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        const circuit::Cell& cell = cells.at(cell_id);
        int lib_cell_id = cell.get_lib_cell_id();
        double new_cell_power_cost = ff_libcell_cost_manager.get_ff_libcell_cost(lib_cell_id).get_power_cost();
        double new_cell_area_cost = ff_libcell_cost_manager.get_ff_libcell_cost(lib_cell_id).get_area_cost();
        add_power_cost(new_cell_power_cost - sequential_cells_cost[cell_id].get_power_cost());
        add_area_cost(new_cell_area_cost - sequential_cells_cost[cell_id].get_area_cost());
        sequential_cells_cost[cell_id].set_power_cost(new_cell_power_cost);
        sequential_cells_cost[cell_id].set_area_cost(new_cell_area_cost);
    }
}

}