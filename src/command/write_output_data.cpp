#include "command_manager.h"
#include <string>
#include <iostream>
#include <fstream>
#include "../design/design.h"
#include "../design/libcell.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../circuit/solution.h"
#include <unordered_set>

namespace command{
void CommandManager::write_output_data(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::ofstream file(filename);    
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    int current_cell_size = 0;

    for(const auto &cell : cells){
        int parent = cell.get_parent();
        if(cell.is_sequential()== false || parent != -1){
            continue;
        }                
        current_cell_size++;
    }        
    file << "CellInst "<<current_cell_size<<std::endl;
    for(const auto &cell : cells){
        if(cell.is_sequential()== false){
            continue;
        }
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
    // mapping    
    const std::vector<std::string> &original_pin_names = netlist.get_original_pin_names();    
    for(const auto &cell : cells){
        if(cell.is_sequential()== false){
            continue;
        }
        int parent = cell.get_parent();
        if(parent != -1){
            continue;
        }
        const std::vector<int> &pins_id = cell.get_pins_id();
        for(int pin_id : pins_id){            
            const std::string &original_pin_name = original_pin_names.at(pin_id);
            const std::string &mapping_pin_name = netlist.get_pin_name(pin_id);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
        }
    }
    file.close();
}

void CommandManager::write_output_data_from_best_solution(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output:"<<filename <<std::endl;
    design::Design &design = design::Design::get_instance();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    circuit::SolutionManager &solution_manager = circuit::SolutionManager::get_instance();
    const circuit::Solution &best_solution = solution_manager.get_best_solution();

    std::ofstream file(filename);
    if(file.is_open() == false){
        std::cout<<"Error: cannot open file:"<<filename<<std::endl;
        return;
    }

    const std::vector<circuit::Cell> &cells = best_solution.get_cells();
    const std::unordered_set<int> &sequential_cells_id = best_solution.get_sequential_cells_id();
    file << "CellInst "<<static_cast<int>(sequential_cells_id.size())<<std::endl;

    for(int cell_id : sequential_cells_id){
        const circuit::Cell &cell = cells.at(cell_id);
        std::string cell_name = netlist.get_cell_name(cell_id);        
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        std::string lib_cell_name = lib_cell.get_name();
        int x = cell.get_x();
        int y = cell.get_y();
        file << "Inst "<<cell_name<<" "<<lib_cell_name<<" "<<x<<" "<<y<<std::endl;
    } 
    // mapping    
    const std::vector<std::string> &original_pin_names = netlist.get_original_pin_names();  
    for(int cell_id : sequential_cells_id){
        const circuit::Cell &cell = cells.at(cell_id);
        std::string cell_name = netlist.get_cell_name(cell_id);
        std::string pin_prefix = cell_name + "/";
        const design::LibCell &lib_cell = design.get_lib_cell(cell.get_lib_cell_id());
        const std::vector<int> &cells_input_pins_id = cell.get_input_pins_id();
        const std::vector<std::string> &input_pins_name = lib_cell.get_input_pins_name();
        int pid = 0;
        for(int pin_id : cells_input_pins_id){
            const std::string &original_pin_name = original_pin_names.at(pin_id);
            const std::string &mapping_pin_name = pin_prefix + input_pins_name.at(pid);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
            pid++;
        }
        pid = 0;

        const std::vector<int> &cells_output_pins_id = cell.get_output_pins_id();
        const std::vector<std::string> &output_pins_name = lib_cell.get_output_pins_name();
        for(int pin_id : cells_output_pins_id){
            const std::string &original_pin_name = original_pin_names.at(pin_id);
            const std::string &mapping_pin_name = pin_prefix + output_pins_name.at(pid);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
            pid++;
        }
        pid = 0;

        const std::vector<int> &cells_other_pins_id = cell.get_other_pins_id();
        const std::vector<std::string> &other_pins_name = lib_cell.get_other_pins_name();
        const std::string &other_pin_name = other_pins_name.at(0);
        for(int pin_id : cells_other_pins_id){
            const std::string &original_pin_name = original_pin_names.at(pin_id);
            const std::string &mapping_pin_name = pin_prefix + other_pin_name;
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;            
        }        
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