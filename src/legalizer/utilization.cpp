#include "utilization.h"
#include "../design/design.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
namespace legalizer {

void UtilizationCalculator::init_bins(){
    const design::Design& design = design::Design::get_instance();
    double bin_width =  design.get_bin_width();
    double bin_height = design.get_bin_height();
    int col = design.get_die_width() / bin_width;
    int row = design.get_die_height() / bin_height;
    std::cout << "Die width: " << design.get_die_width() << " Die height: " << design.get_die_height() << std::endl;
    std::cout << "Init bins row: " << row <<" col: " << col << std::endl;
    bins.resize(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            bins.at(i).push_back(design::Bin(j * bin_width, i * bin_height, bin_width, bin_height));
        }
    }
    overflow_bins = 0;
}

void UtilizationCalculator::reset_bins_utilization(){
    for(auto& bin_row : bins){
        for(auto& bin : bin_row){
            bin.set_utilization(0.0);
        }
    }
    overflow_bins = 0;
}

void UtilizationCalculator::update_bins_utilization(){
    reset_bins_utilization();

    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const design::Design& design = design::Design::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    double bin_area = design.get_bin_area();
    double bin_width = design.get_bin_width();
    double bin_height = design.get_bin_height();
    for(const auto& cell : cells){
        double x = cell.get_x();
        double y = cell.get_y();
        double rx = cell.get_rx();
        double ry = cell.get_ry();    

        // Calculate the range of bins that the cell overlaps with
        int start_row = std::max(0, static_cast<int>(y / bin_height));
        int end_row = std::min(static_cast<int>(bins.size()) - 1, static_cast<int>((ry) / bin_height));
        int start_col = std::max(0, static_cast<int>(x / bin_width));
        int end_col = std::min(static_cast<int>(bins[0].size()) - 1, static_cast<int>((rx) / bin_width));

        // Add the overlap area to bin utilization
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                double overlap_area = (std::min((j + 1) * bin_width, rx) - std::max(j * bin_width, x)) *
                          (std::min((i + 1) * bin_height, ry) - std::max(i * bin_height, y));
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