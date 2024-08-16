#include "lib_cell_evaluator.h"
#include "../design/design.h"
#include <algorithm>
namespace estimator{

void FFLibcellCostManager::calculate_cost(){
    const design::Design& design = design::Design::get_instance();    
    const std::vector<design::LibCell>& libcells = design.get_lib_cells();
    double timing_factor = design.get_timing_factor();
    double power_factor = design.get_power_factor();
    double area_factor = design.get_area_factor();

    for(int i=0;i<libcells.size();i++){
        // ignore slack first        
        bool is_sequential = libcells[i].is_sequential();
        if(!is_sequential){
            continue;
        }

        int bits = libcells[i].get_bits();
        // ESTIMATE COST
        double timing_cost = libcells[i].get_delay() * bits * 0.0;  /*timing_factor;*/
        // REAL COST
        double power_cost = libcells[i].get_power() * power_factor;
        double area_cost = libcells[i].get_area() * area_factor;
        bits_ff_libcells_cost[bits].push_back(FFLibCellCost(i,timing_cost,power_cost,area_cost));        
    }
}

void FFLibcellCostManager::sort_by_cost(){
    bits_ff_libcells_sort_by_total_cost = bits_ff_libcells_cost;    
    for(auto& it : bits_ff_libcells_sort_by_total_cost){
        std::sort(it.second.begin(),it.second.end(),[](const FFLibCellCost& a, const FFLibCellCost& b){
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


}