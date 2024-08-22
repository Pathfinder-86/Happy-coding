#include "utilization.h"
#include "../design/design.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/original_netlist.h"
#include "../circuit/cell.h"
#include <cmath>
namespace legalizer {

void UtilizationCalculator::init_utilization_calculator(){
    const design::Design& design = design::Design::get_instance();
    double bin_width =  design.get_bin_width();
    double bin_height = design.get_bin_height();
    int bin_width_int = bin_width;
    int bin_height_int = bin_height;
    int col = design.get_die_width() / bin_width;
    int row = design.get_die_height() / bin_height;
    std::cout << "Die width: " << design.get_die_width() << " Die height: " << design.get_die_height() << std::endl;
    std::cout << "Init bins row: " << row <<" col: " << col << std::endl;
    init_bins.resize(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            init_bins.at(i).push_back(design::Bin(j * bin_width, i * bin_height, bin_width, bin_height));
        }
    }

    const circuit::OriginalNetlist& netlist = circuit::OriginalNetlist::get_instance();
    for(const circuit::Cell &cell : netlist.get_cells()){
        if(cell.is_sequential()){
            continue;
        }
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();        
        int start_row = y / bin_height_int;
        int end_row = ry % bin_height_int == 0? (ry / bin_height_int) -1 : ry / bin_height_int;
        int start_col = x / bin_width_int;
        int end_col = rx % bin_width_int == 0? (rx / bin_width_int) -1 : rx / bin_width_int;
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                // calculate the overlap area                
                double overlap_area = 1.0 * (std::min((j + 1) * bin_width_int, rx) - std::max(j * bin_width_int, x)) * 
                          (std::min((i + 1) * bin_height_int, ry) - std::max(i * bin_height_int, y));
                init_bins[i][j].add_utilization(overlap_area / design.get_bin_area());
            }
        }
    }
    double max_utilization = design.get_bin_max_utilization();
    init_overflow_bins = 0;
    for(auto& bin_row : init_bins){
        for(auto& bin : bin_row){
            if(bin.get_utilization() >= max_utilization){
                init_overflow_bins++;
            }
        }
    }
}

void UtilizationCalculator::reset_bins_utilization(){
    bins = init_bins;
    overflow_bins = init_overflow_bins;
}

void UtilizationCalculator::update_bins_utilization(){
    reset_bins_utilization();

    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const design::Design& design = design::Design::get_instance();
    const std::unordered_set<int>& sequential_cells_id = netlist.get_sequential_cells_id();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    double bin_area = design.get_bin_area();
    double bin_width = design.get_bin_width();
    double bin_height = design.get_bin_height();
    int bin_width_int = bin_width;
    int bin_height_int = bin_height;
    for(int cell_id : sequential_cells_id){
        const circuit::Cell& cell = cells.at(cell_id);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();    

        // Calculate the range of bins that the cell overlaps with
        int start_row = y / bin_height_int;
        int end_row = ry % bin_height_int == 0? (ry / bin_height_int) -1 : ry / bin_height_int;
        int start_col = x / bin_width_int;
        int end_col = rx % bin_width_int == 0? (rx / bin_width_int) -1 : rx / bin_width_int;
        // Add the overlap area to bin utilization
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                double overlap_area = 1.0 * (std::min((j + 1) * bin_width_int, rx) - std::max(j * bin_width_int, x)) *
                          (std::min((i + 1) * bin_height_int, ry) - std::max(i * bin_height_int, y));
                bins[i][j].add_utilization(overlap_area / bin_area);
            }
        }        
    }
    double max_utilization = design.get_bin_max_utilization();
    for(auto& bin_row : bins){
        for(auto& bin : bin_row){
            if(bin.get_utilization() >= max_utilization){
                overflow_bins++;
            }
        }
    }
}


}