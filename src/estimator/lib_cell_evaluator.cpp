#include "lib_cell_evaluator.h"
#include "../design/design.h"
#include <algorithm>
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
namespace estimator{

double FFLibcellCostManager::get_cluster_cost_change(const std::vector<int> &cells_id) const{
    double cost_change = 0.0;
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    int sum_bits = 0;
    for(int i=0;i<cells_id.size();i++){
        int cell_id = cells_id[i];
        const circuit::Cell& cell = cells.at(cell_id);
        int lib_cell_id = cell.get_lib_cell_id();
        double cost = get_lib_cell_cost(lib_cell_id);        
        cost_change -= cost;        
        sum_bits+=cell.get_bits();
    }
    int cluster_lib_cell_id = get_best_libcell_for_bit(sum_bits);
    double cluster_cost = get_lib_cell_cost(cluster_lib_cell_id);
    cost_change += cluster_cost;    
    return cost_change;
}

void FFLibcellCostManager::calculate_cost(){
    const design::Design& design = design::Design::get_instance();    
    const std::vector<design::LibCell>& libcells = design.get_lib_cells();    
    double power_factor = design.get_power_factor();
    double area_factor = design.get_area_factor();

    for(int i=0;i<libcells.size();i++){
        // ignore slack first        
        bool is_sequential = libcells[i].is_sequential();
        if(!is_sequential){
            continue;
        }

        int bits = libcells[i].get_bits();
        bits_num.insert(bits);
        // delay
        double timing_cost = libcells[i].get_delay();
        // REAL COST
        double power_cost = libcells[i].get_power() * power_factor;
        double area_cost = libcells[i].get_area() * area_factor;
        bits_ff_libcells_cost[bits].push_back(FFLibCellCost(i,timing_cost,power_cost,area_cost));
        ff_libcells_cost.insert({i,FFLibCellCost(i,timing_cost,power_cost,area_cost)});
    }
}

void FFLibcellCostManager::sort_by_cost(){
    bits_ff_libcells_sort_by_total_cost = bits_ff_libcells_cost;    
    for(auto& it : bits_ff_libcells_sort_by_total_cost){
        std::sort(it.second.begin(),it.second.end(),[](const FFLibCellCost& a, const FFLibCellCost& b){
            if(a.get_total_cost() == b.get_total_cost()){
                return a.get_timing_cost() < b.get_timing_cost();
            }
            return a.get_total_cost() < b.get_total_cost();
        });        
    }

    bits_ff_libcells_sort_by_power_cost = bits_ff_libcells_cost;
    for(auto& it : bits_ff_libcells_sort_by_power_cost){
        std::sort(it.second.begin(),it.second.end(),[](const FFLibCellCost& a, const FFLibCellCost& b){
            return a.get_power_cost() < b.get_power_cost();
        });
    }

    bits_ff_libcells_sort_by_area_cost = bits_ff_libcells_cost;
    for(auto& it : bits_ff_libcells_sort_by_area_cost){
        std::sort(it.second.begin(),it.second.end(),[](const FFLibCellCost& a, const FFLibCellCost& b){
            return a.get_area_cost() < b.get_area_cost();
        });
    }

    bits_ff_libcells_sort_by_timing_cost = bits_ff_libcells_cost;
    for(auto& it : bits_ff_libcells_sort_by_timing_cost){
        std::sort(it.second.begin(),it.second.end(),[](const FFLibCellCost& a, const FFLibCellCost& b){
            return a.get_timing_cost() < b.get_timing_cost();
        });
    }
}

int FFLibcellCostManager::find_mid_bits_of_lib(){
    std::vector<int> bits = std::vector<int>(bits_num.begin(),bits_num.end());
    std::sort(bits.begin(),bits.end());    
    return bits[bits.size()/2];
}

void FFLibcellCostManager::find_best_libcell_bits(){
    std::vector<std::pair<int,double>> best_libcell_for_bits_avg_cost_per_bit;
    for(auto& it : bits_ff_libcells_sort_by_total_cost){
        int bits = it.first;
        int best_libcell_id = it.second.front().get_id();
        best_libcell_bits[bits] = best_libcell_id;        
        double total_cost = it.second.front().get_total_cost();
        best_libcell_for_bits_avg_cost_per_bit.push_back(std::make_pair(bits,total_cost/bits));
    }
    std::sort(best_libcell_for_bits_avg_cost_per_bit.begin(),best_libcell_for_bits_avg_cost_per_bit.end(),[](const std::pair<int,double>& a, const std::pair<int,double>& b){
        return a.second < b.second;
    });
    for(int i=0;i<best_libcell_for_bits_avg_cost_per_bit.size();i++){
        best_libcell_sorted_by_bits.push_back(best_libcell_for_bits_avg_cost_per_bit[i].first);        
    }

    // DEBUG
    std::cout<<"FFLibcellCostManager::find_best_libcell_bits"<<std::endl;
    const design::Design& design = design::Design::get_instance();
    for(int bits : best_libcell_sorted_by_bits){
        int lib_cell_id = best_libcell_bits[bits];
        const std::string &lib_cell_name = design.get_lib_cells().at(lib_cell_id).get_name();
        std::cout<<"bits: "<<bits<<" best_lib_cell: "<<lib_cell_name<<std::endl;
    }
}


}