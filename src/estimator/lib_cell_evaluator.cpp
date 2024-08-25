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
        ff_libcells_cost.insert({i,FFLibCellCost(i,timing_cost,power_cost,area_cost)});
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

int FFLibcellCostManager::find_mid_bits_of_lib(){
    std::sort(bits_num.begin(),bits_num.end());
    int mid = bits_num.size()/2;
    return bits_num[mid];
}

void FFLibcellCostManager::find_best_libcell_bits(){
    std::vector<std::pair<int,double>> best_libcell_for_bits_avg_cost_per_bit;
    for(auto& it : bits_ff_libcells_sort_by_total_cost){
        int bits = it.first;
        int best_libcell_id = it.second.front().get_id();
        best_libcell_bits[bits] = best_libcell_id;
        bits_num.push_back(bits);
        double total_cost = it.second.front().get_total_cost();
        best_libcell_for_bits_avg_cost_per_bit.push_back(std::make_pair(bits,total_cost/bits));
    }
    std::sort(best_libcell_for_bits_avg_cost_per_bit.begin(),best_libcell_for_bits_avg_cost_per_bit.end(),[](const std::pair<int,double>& a, const std::pair<int,double>& b){
        return a.second < b.second;
    });
    for(int i=0;i<best_libcell_for_bits_avg_cost_per_bit.size();i++){
        best_libcell_sorted_by_bits.push_back(best_libcell_for_bits_avg_cost_per_bit[i].first);        
    }
}


}