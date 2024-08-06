#include "design.h"
#include <iostream>
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../timer/timer.h"

namespace design {
    
void Design::init_bins(){
    double bin_width =  get_bin_width();
    double bin_height = get_bin_height();
    int col = get_die_width() / bin_width;
    int row = get_die_height() / bin_height;
    std::cout << "Die width: " << get_die_width() << " Die height: " << get_die_height() << std::endl;
    std::cout << "Init bins row: " << row <<" col: " << col << std::endl;
    bins.resize(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            bins.at(i).push_back(Bin(j * bin_width, i * bin_height, bin_width, bin_height));            
        }
    }
}

void Design::update_bins_utilization(){
    for(auto& bin_row : bins){
        for(auto& bin : bin_row){
            bin.set_utilization(0.0);
        }
    }
    circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    double bin_area = get_bin_width() * get_bin_height();
    for(auto& cell : cells){
        double x = cell.get_x();
        double y = cell.get_y();
        double rx = cell.get_rx();
        double ry = cell.get_ry();    

        // Calculate the range of bins that the cell overlaps with
        int start_row = std::max(0, static_cast<int>(y / get_bin_height()));
        int end_row = std::min(static_cast<int>(bins.size()) - 1, static_cast<int>((ry) / get_bin_height()));
        int start_col = std::max(0, static_cast<int>(x / get_bin_width()));
        int end_col = std::min(static_cast<int>(bins[0].size()) - 1, static_cast<int>((rx) / get_bin_width()));

        // Add the overlap area to bin utilization
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
            double overlap_area = (std::min((j + 1) * get_bin_width(), rx) - std::max(j * get_bin_width(), x)) *
                          (std::min((i + 1) * get_bin_height(), ry) - std::max(i * get_bin_height(), y));
            bins[i][j].add_utilization(overlap_area / bin_area);
            }
        }        
    }
}

void Design::calculate_cost(){
    this->cost.reset();    
    double timing_factor = get_timing_factor();
    double power_factor = get_power_factor();
    double area_factor = get_area_factor();
    double utilization_factor = get_utilization_factor();

    circuit::Netlist& netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell>& cells = netlist.get_cells();
    for(auto& cell : cells){
        if(cell.is_sequential() == false){
            continue;
        }
        int id = cell.get_id();
        const std::string &cell_name = netlist.get_cell_name(id);
        this->cost.area_cost += area_factor * cell.get_area();
        this->cost.power_cost += power_factor * cell.get_power();
        this->cost.timing_cost += timing_factor * cell.get_slack();
        std::cout<<"cell: "<<cell_name<<std::endl;
        std::cout<<"area: "<<cell.get_area()<<" "<<cell.get_area() * area_factor<<std::endl;
        std::cout<<"power: "<<cell.get_power()<<" "<<cell.get_power() * power_factor<<std::endl;
        std::cout<<"slack: "<<cell.get_slack()<<" "<<cell.get_slack() * timing_factor<<std::endl;
    }
    // TODO:  add utilization cost
    this->cost.utilization_cost = utilization_factor * 0.0;

    this->cost.total_cost = this->cost.area_cost + this->cost.power_cost + this->cost.timing_cost + this->cost.utilization_cost;    
}


}