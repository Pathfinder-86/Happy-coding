#include "command_manager.h"
#include <string>
#include <iostream>
#include <fstream>
#include "../design/design.h"
#include "../design/libcell.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"

namespace command{
void CommandManager::write_output_data(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::ofstream file(filename);    
    std::vector<circuit::Cell> cells = netlist.get_cells();
    for(const auto &cell : cells){
        int parent = cell.get_parent();
        if(parent != -1){
            continue;
        }
        int id = cell.get_id();
        std::string cell_name = netlist.get_cell_name(id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        std::string lib_cell_name = lib_cell.get_name();
        int x = cell.get_x();
        int y = cell.get_y();
        file << "Inst "<<cell_name<<" "<<lib_cell_name<<" "<<x<<" "<<y<<std::endl;
    }    
    file.close();
}

void CommandManager::write_output_layout_data(const std::string &filename) {    
    std::cout<<"write_output_layout_data:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::ofstream file(filename);
    std::vector<double> die_boundaries = design.get_die_boundaries();
    file << "Die "<<(int)die_boundaries.at(0)<<" "<<(int)die_boundaries.at(1)<<" "<<(int)die_boundaries.at(2)<<" "<<(int)die_boundaries.at(3)<<std::endl;
    // CELL
    int row_height = design.get_bin_height();
    file << "RowHeight "<<row_height<<std::endl;
    std::vector<circuit::Cell> cells = netlist.get_cells();
    for(const auto &cell : cells){
        int parent = cell.get_parent();
        if(parent != -1){
            continue;
        }
        int id = cell.get_id();
        std::string cell_name = netlist.get_cell_name(id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        bool is_flipflop = lib_cell.is_sequential();
        std::string lib_cell_name = lib_cell.get_name();
        std::string cell_type = is_flipflop ? "FF" : "other";
        file << "Inst "<<cell_name<<" "<<cell_type<<" "<<(int)cell.get_x()<<" "<<(int)cell.get_y()<<" "<<(int)cell.get_rx()<<" "<<(int)cell.get_ry()<<std::endl;
    }    
    file.close();
}


void CommandManager::write_output_utilization_data(const std::string &filename) {    
    std::cout<<"write_output_utilization_data:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();    
    std::ofstream file(filename);
    std::vector<double> die_boundaries = design.get_die_boundaries();
    file << "Die "<<(int)die_boundaries.at(0)<<" "<<(int)die_boundaries.at(1)<<" "<<(int)die_boundaries.at(2)<<" "<<(int)die_boundaries.at(3)<<std::endl;    
    std::vector<std::vector<design::Bin>> bins = design.get_bins();
    for(const auto &row : bins){
        for(const auto &bin : row){
            file << "Bin "<<bin.get_x()<<" "<<bin.get_y()<<" "<<bin.get_rx()<<" "<<bin.get_ry()<<" "<<bin.get_utilization()<<std::endl;
        }
    }
    file.close();
}


}