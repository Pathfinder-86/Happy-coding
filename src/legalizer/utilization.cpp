#include "utilization.h"
#include "../design/design.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/original_netlist.h"
#include "../circuit/cell.h"
#include <cmath>
#include <iostream>
namespace legalizer {

void UtilizationCalculator::init_utilization_calculator(){
    const design::Design& design = design::Design::get_instance();
    double bin_width =  design.get_bin_width();
    double bin_height = design.get_bin_height();
    this->bin_area = bin_width * bin_height;
    this->bin_width_int = bin_width;
    this->bin_height_int = bin_height;

    int col = design.get_die_width() / bin_width;
    int row = design.get_die_height() / bin_height;
    std::cout << "Die width: " << design.get_die_width() << " Die height: " << design.get_die_height() << std::endl;
    std::cout << "Init bins row: " << row <<" col: " << col << std::endl;
    init_bins.resize(row);
    bins.resize(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            init_bins.at(i).push_back(design::Bin(j * bin_width, i * bin_height, bin_width, bin_height));
            bins.at(i).push_back(design::Bin(j * bin_width, i * bin_height, bin_width, bin_height));
        }
    }

    const circuit::OriginalNetlist& netlist = circuit::OriginalNetlist::get_instance();
    for(const circuit::Cell &cell : netlist.get_cells()){
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();        
        int start_row = y / bin_height_int;
        int end_row = ry % bin_height_int == 0? (ry / bin_height_int) -1 : ry / bin_height_int;
        int start_col = x / get_bin_width_int();
        int end_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                // calculate the overlap area                
                double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) * 
                          (std::min((i + 1) * bin_height_int, ry) - std::max(i * bin_height_int, y));
                double utilization = overlap_area / design.get_bin_area();
                if(!cell.is_sequential()){
                    init_bins[i][j].add_utilization(utilization);
                }                
                bins[i][j].add_utilization(utilization);                
            }
        }
    }
    this->max_utilization = design.get_bin_max_utilization();
    std::cout<<"UTIL max_utilization: "<<max_utilization<<std::endl;
    for(int i=0; i<row; i++){
        for(int j=0; j<col; j++){
            if(is_overflow_bin(i, j)){
                overflow_bins_id.insert(std::make_pair(i, j));
                std::cout<<"UTIL init bins has already overflow ("<<i<<","<<j<<"), may need to remove sites, and replacement ff on related-sites"<<std::endl;
            }            
        }
    }

    print();
}

void UtilizationCalculator::reset_bins_utilization(){
    bins = init_bins;
    overflow_bins_id.clear();
}

void UtilizationCalculator::update_bins_utilization(){
    reset_bins_utilization();

    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::unordered_set<int>& sequential_cells_id = netlist.get_sequential_cells_id();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    
    for(int cell_id : sequential_cells_id){
        const circuit::Cell& cell = cells.at(cell_id);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();    

        // Calculate the range of bins that the cell overlaps with
        int start_row = y / get_bin_height_int();
        int end_row = ry % get_bin_height_int() == 0? (ry / get_bin_height_int()) -1 : ry / get_bin_height_int();
        int start_col = x / get_bin_width_int();
        int end_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();
        // Add the overlap area to bin utilization
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) *
                          (std::min((i + 1) * get_bin_height_int(), ry) - std::max(i * get_bin_height_int(), y));
                double utilization = overlap_area / get_bin_area();
                bins[i][j].add_utilization(overlap_area / bin_area);
            }
        }        
    }

    for(int i=0; i<bins.size(); i++){
        for(int j=0; j<bins[i].size(); j++){
            if(is_overflow_bin(i, j)){
                overflow_bins_id.insert(std::make_pair(i, j));
            }
        }
    }
}

void UtilizationCalculator::remove_cells(const std::vector<int> &cells_id){
    for(int cell_id : cells_id){
        remove_cell(cell_id);
    }
}

void UtilizationCalculator::add_cells(const std::vector<int> &cells_id){
    for(int cell_id : cells_id){
        add_cell(cell_id);
    }
}

void UtilizationCalculator::add_cell(int cell_id) {
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const circuit::Cell& cell = netlist.get_cells().at(cell_id);
    int x = cell.get_x();
    int y = cell.get_y();
    int rx = cell.get_rx();
    int ry = cell.get_ry();

    int start_bin_row = y / get_bin_height_int();
    int end_bin_row = ry % get_bin_height_int() == 0? (ry / get_bin_height_int()) -1 : ry / get_bin_height_int();
    int start_bin_col = x / get_bin_width_int();
    int end_bin_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();
    for (int i = start_bin_row; i <= end_bin_row; i++) {
        for (int j = start_bin_col; j <= end_bin_col; j++) {
            double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) *
                      (std::min((i + 1) * get_bin_height_int(), ry) - std::max(i * get_bin_height_int(), y));
            double utilization = overlap_area / design::Design::get_instance().get_bin_area();
            //std::cout<<"UTIL bin("<<i<<","<<j<<") before utilization: "<<bins[i][j].get_utilization()<<" add utilization"<<utilization<<std::endl;
            bins[i][j].add_utilization(utilization);
            //std::cout<<"UTIL bin("<<i<<","<<j<<") after utilization: "<<bins[i][j].get_utilization()<<std::endl;
            if( !overflow_bins_id.count(std::make_pair(i, j)) && is_overflow_bin(i, j)){
                overflow_bins_id.insert(std::make_pair(i, j));
                //std::cout<<"UTIL bin("<<i<<","<<j<<") new overflow bin"<<std::endl;
            }
        }
    }
}

void UtilizationCalculator::remove_cell(int cell_id) {
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const circuit::Cell& cell = netlist.get_cells().at(cell_id);
    int x = cell.get_x();
    int y = cell.get_y();
    int rx = cell.get_rx();
    int ry = cell.get_ry();

    int start_bin_row = y / get_bin_height_int();
    int end_bin_row = ry % get_bin_height_int() == 0? (ry / get_bin_height_int()) -1 : ry / get_bin_height_int();
    int start_bin_col = x / get_bin_width_int();
    int end_bin_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();
    for (int i = start_bin_row; i <= end_bin_row; i++) {
        for (int j = start_bin_col; j <= end_bin_col; j++) {
            double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) *
                      (std::min((i + 1) * get_bin_height_int(), ry) - std::max(i * get_bin_height_int(), y));
            double utilization = overlap_area / design::Design::get_instance().get_bin_area();
            //std::cout<<"UTIL bin("<<i<<","<<j<<") before remove cell utilization: "<<bins[i][j].get_utilization()<<" remove utilization"<<utilization<<std::endl;
            bins[i][j].add_utilization( -utilization );
            //std::cout<<"UTIL bin("<<i<<","<<j<<") after remove cell utilization: "<<bins[i][j].get_utilization()<<std::endl;
            if( overflow_bins_id.count(std::make_pair(i, j))  && !is_overflow_bin(i, j)){
                overflow_bins_id.erase(std::make_pair(i, j));
                //std::cout<<"UTIL bin("<<i<<","<<j<<") remove overflow bin"<<std::endl;                
            }
        }
    }
}

double UtilizationCalculator::add_cell_cost_change(int cell_id) {
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const circuit::Cell& cell = netlist.get_cells().at(cell_id);
    const design::Design& design = design::Design::get_instance();
    int x = cell.get_x();
    int y = cell.get_y();
    int rx = cell.get_rx();
    int ry = cell.get_ry();

    int start_bin_row = y / get_bin_height_int();
    int end_bin_row = ry % get_bin_height_int() == 0? (ry / get_bin_height_int()) -1 : ry / get_bin_height_int();
    int start_bin_col = x / get_bin_width_int();
    int end_bin_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();    
    int new_overflow_bins = 0;
    for (int i = start_bin_row; i <= end_bin_row; i++) {
        for (int j = start_bin_col; j <= end_bin_col; j++) {
            double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) *
                      (std::min((i + 1) * get_bin_height_int(), ry) - std::max(i * get_bin_height_int(), y));
            double utilization = overlap_area / design.get_bin_area();
            //std::cout<<"UTIL bin("<<i<<","<<j<<") before utilization: "<<bins[i][j].get_utilization()<<" add utilization"<<utilization<<std::endl;
            bins[i][j].add_utilization(utilization);
            //std::cout<<"UTIL bin("<<i<<","<<j<<") after utilization: "<<bins[i][j].get_utilization()<<std::endl;
            if(is_overflow_bin(i, j) && !overflow_bins_id.count(std::make_pair(i, j))){
                overflow_bins_id.insert(std::make_pair(i, j));                
                new_overflow_bins++;
            }            
        }
    }
    const double utilization_factor = design.get_utilization_factor();
    return new_overflow_bins * utilization_factor;
}

double UtilizationCalculator::remove_cells_cost_change(const std::vector<int> &cells_id) {
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const design::Design& design = design::Design::get_instance();
    int delete_overflow_bins = 0;
    for(int cell_id : cells_id){
        const circuit::Cell& cell = netlist.get_cells().at(cell_id);
        const design::Design& design = design::Design::get_instance();
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();

        int start_bin_row = y / get_bin_height_int();
        int end_bin_row = ry % get_bin_height_int() == 0? (ry / get_bin_height_int()) -1 : ry / get_bin_height_int();
        int start_bin_col = x / get_bin_width_int();
        int end_bin_col = rx % get_bin_width_int() == 0? (rx / get_bin_width_int()) -1 : rx / get_bin_width_int();            
        for (int i = start_bin_row; i <= end_bin_row; i++) {
            for (int j = start_bin_col; j <= end_bin_col; j++) {
                double overlap_area = 1.0 * (std::min((j + 1) * get_bin_width_int(), rx) - std::max(j * get_bin_width_int(), x)) *
                            (std::min((i + 1) * get_bin_height_int(), ry) - std::max(i * get_bin_height_int(), y));
                double utilization = overlap_area / design.get_bin_area();                
                bins[i][j].add_utilization( -utilization);
                if( overflow_bins_id.count(std::make_pair(i, j)) && !is_overflow_bin(i, j)){
                    overflow_bins_id.erase(std::make_pair(i, j));                    
                    delete_overflow_bins++;
                }            
            }
        }
    }
    const double utilization_factor = design.get_utilization_factor();
    return -delete_overflow_bins * utilization_factor;
}

void UtilizationCalculator::print(){
    std::cout<<"UTILIZATION::"<<std::endl;
    for(int i=0; i<bins.size(); i++){
        for(int j=0; j<bins[i].size(); j++){
            std::cout<<"("<<i<<","<<j<<") "<<bins[i][j].get_utilization()<<" ";
        }
        std::cout<<std::endl;
    }
    std::cout<<"OVERFLOW BINS::"<<std::endl;
    for(auto it : overflow_bins_id){
        std::cout<<"("<<it.first<<","<<it.second<<") ";
    }
    std::cout<<std::endl;
}



}