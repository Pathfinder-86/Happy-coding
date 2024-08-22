#include "netlist.h"
#include <iostream>
#include <../timer/timer.h>
#include <../estimator/lib_cell_evaluator.h>
#include <../legalizer/legalizer.h>
#include "original_netlist.h"
namespace circuit {

bool OriginalNetlist::check_cells_location(const std::vector<Cell> &removed_clustered_cells){
    if(check_overlap(removed_clustered_cells)){
        return true;
    }
    if(check_out_of_die(removed_clustered_cells)){
        return true;
    }
    return false;
}

bool OriginalNetlist::check_overlap(const std::vector<Cell> &removed_clustered_cells){
    // check seqs
    for(int i=0;i<static_cast<int>(removed_clustered_cells.size());i++){
        const Cell &ff_cell1 = cells.at(i);
        for(int j=i+1;j<static_cast<int>(removed_clustered_cells.size());j++){
            const Cell &ff_cell2 = removed_clustered_cells.at(j);
            if(ff_cell1.overlap(ff_cell2)){        
                std::cout<<"ERROR! seqs overlap";        
                return true;
            }
        }
    }

    // check seq and comb
    for(const auto &ff_cell :removed_clustered_cells){
        for(const auto &comb_cell: comb_cells){
            if(ff_cell.overlap(comb_cell)){
                std::cout<<"ERROR! seq and comb overlap";
                return true;
            }
        }

    }
    return false;
}

bool OriginalNetlist::check_out_of_die(const std::vector<Cell> &removed_clustered_cells){
    const design::Design &design = design::Design::get_instance();
    const std::vector<double> &die_boundaries = design.get_die_boundaries();
    for(const Cell &cell: removed_clustered_cells){
        if(cell.get_x() < die_boundaries.at(0) || cell.get_rx() > die_boundaries.at(2)){
            return true;
        }
        if(cell.get_y() < die_boundaries.at(1) || cell.get_ry() > die_boundaries.at(3)){
            return true;
        }
    }
    return false;
}

}