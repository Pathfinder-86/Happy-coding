#include "command_manager.h"
#include <string>
#include <iostream>
#include <fstream>
#include "../design/design.h"
#include "../design/libcell.h"
#include "../design/bin.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include "../estimator/solution.h"
#include <unordered_set>
#include "../legalizer/utilization.h"
#include "../circuit/original_netlist.h"

namespace command{
void CommandManager::write_output_data(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output:"<<filename<<std::endl;
    const design::Design &design = design::Design::get_instance();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    std::ofstream file(filename);    
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    int current_cell_size = 0;

    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        current_cell_size++;
    }        
    file << "CellInst "<<current_cell_size<<std::endl;
    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        int id = cell.get_id();
        const std::string &cell_name = "C" + std::to_string(id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        std::string lib_cell_name = lib_cell.get_name();
        int x = cell.get_x();
        int y = cell.get_y();
        file << "Inst "<<cell_name<<" "<<lib_cell_name<<" "<<x<<" "<<y<<std::endl;
    } 
    // mapping    
    const std::vector<std::string> &original_pin_names = original_netlist.get_pin_names();    
    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        int cell_id = cell.get_id();
        const std::string &cell_name = "C" + std::to_string(cell_id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        const std::vector<int> &cells_input_pins_id = cell.get_input_pins_id();
        const std::vector<std::string> &input_pins_name = lib_cell.get_input_pins_name();
        for(int i = 0; i < static_cast<int>(cells_input_pins_id.size()); i++){
            int ff_pin_id = cells_input_pins_id.at(i);
            int original_pin_id = original_netlist.get_original_pin_id(ff_pin_id);
            const std::string &original_pin_name = original_pin_names.at(original_pin_id);
            const std::string &mapping_pin_name = cell_name + "/" + input_pins_name.at(i);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
        }
        const std::vector<int> &cells_output_pins_id = cell.get_output_pins_id();
        const std::vector<std::string> &output_pins_name = lib_cell.get_output_pins_name();
        std::unordered_set<int> original_ff_cells_id;
        for(int i = 0; i < static_cast<int>(cells_output_pins_id.size()); i++){
            int ff_pin_id = cells_output_pins_id.at(i);
            int original_pin_id = original_netlist.get_original_pin_id(ff_pin_id);
            const std::string &original_pin_name = original_pin_names.at(original_pin_id);
            const std::string &mapping_pin_name = cell_name + "/" + output_pins_name.at(i);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
            const circuit::Pin &original_pin = original_netlist.get_pin(original_pin_id);
            int original_cell_id = original_pin.get_cell_id();
            original_ff_cells_id.insert(original_cell_id);
        }
        // OTHER
        for(int original_cell_id : original_ff_cells_id){
            const std::string &original_cell_name = original_netlist.get_cell_name(original_cell_id);
            const std::string &original_pin_name = original_cell_name + "/CLK";
            const std::string &mapping_pin_name = cell_name + "/CLK";
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
        }
    }
    file.close();
}

void CommandManager::write_output_data_from_best_solution(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output:"<<filename<<std::endl;
    const design::Design &design = design::Design::get_instance();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    std::ofstream file(filename);    
    const std::vector<circuit::Cell> &cells = solution_manager.get_best_solution().get_cells();
    int current_cell_size = 0;

    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        current_cell_size++;
    }        
    file << "CellInst "<<current_cell_size<<std::endl;
    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        int id = cell.get_id();
        const std::string &cell_name = "C" + std::to_string(id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        std::string lib_cell_name = lib_cell.get_name();
        int x = cell.get_x();
        int y = cell.get_y();
        file << "Inst "<<cell_name<<" "<<lib_cell_name<<" "<<x<<" "<<y<<std::endl;
    } 
    // mapping    
    const std::vector<std::string> &original_pin_names = original_netlist.get_pin_names();    
    for(const auto &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        int cell_id = cell.get_id();
        const std::string &cell_name = "C" + std::to_string(cell_id);
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
        const std::vector<int> &cells_input_pins_id = cell.get_input_pins_id();
        const std::vector<std::string> &input_pins_name = lib_cell.get_input_pins_name();
        for(int i = 0; i < static_cast<int>(cells_input_pins_id.size()); i++){
            int ff_pin_id = cells_input_pins_id.at(i);
            int original_pin_id = original_netlist.get_original_pin_id(ff_pin_id);
            const std::string &original_pin_name = original_pin_names.at(original_pin_id);
            const std::string &mapping_pin_name = cell_name + "/" + input_pins_name.at(i);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
        }
        const std::vector<int> &cells_output_pins_id = cell.get_output_pins_id();
        const std::vector<std::string> &output_pins_name = lib_cell.get_output_pins_name();
        std::unordered_set<int> original_ff_cells_id;
        for(int i = 0; i < static_cast<int>(cells_output_pins_id.size()); i++){
            int ff_pin_id = cells_output_pins_id.at(i);
            int original_pin_id = original_netlist.get_original_pin_id(ff_pin_id);
            const std::string &original_pin_name = original_pin_names.at(original_pin_id);
            const std::string &mapping_pin_name = cell_name + "/" + output_pins_name.at(i);
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
            const circuit::Pin &original_pin = original_netlist.get_pin(original_pin_id);
            int original_cell_id = original_pin.get_cell_id();
            original_ff_cells_id.insert(original_cell_id);
        }
        // OTHER
        for(int original_cell_id : original_ff_cells_id){
            const std::string &original_cell_name = original_netlist.get_cell_name(original_cell_id);
            const std::string &original_pin_name = original_cell_name + "/CLK";
            const std::string &mapping_pin_name = cell_name + "/CLK";
            file<<original_pin_name<<" map "<<mapping_pin_name<<std::endl;
        }
    }
    file.close();
}


void CommandManager::write_output_layout_data(const std::string &filename) {    
    std::cout<<"write_output_layout_data:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    std::ofstream file(filename);
    std::vector<double> die_boundaries = design.get_die_boundaries();
    file << "Die "<<(int)die_boundaries.at(0)<<" "<<(int)die_boundaries.at(1)<<" "<<(int)die_boundaries.at(2)<<" "<<(int)die_boundaries.at(3)<<std::endl;
    // CELL
    int row_height = design.get_bin_height();
    file << "RowHeight "<<row_height<<std::endl;
    const std::vector<circuit::Cell> &comb_cells = original_netlist.get_cells();
    const std::vector<circuit::Cell> &ff_cells = netlist.get_cells();
    for(const auto &cell : comb_cells){
        if(cell.is_sequential()){
            continue;
        }
        int id = cell.get_id();
        const std::string &cell_name = "GATE" + std::to_string(id);
        file << "Inst "<<cell_name<<" other"<<" "<<(int)cell.get_x()<<" "<<(int)cell.get_y()<<" "<<(int)cell.get_rx()<<" "<<(int)cell.get_ry()<<std::endl;
    }    
    for(const auto &cell : ff_cells){
        if(cell.is_clustered()){
            continue;
        }
        int id = cell.get_id();
        const std::string &cell_name = "FF" + std::to_string(id);
        file << "Inst "<<cell_name<<" FF"<<" "<<(int)cell.get_x()<<" "<<(int)cell.get_y()<<" "<<(int)cell.get_rx()<<" "<<(int)cell.get_ry()<<std::endl;
    }
    file.close();
}


void CommandManager::write_output_utilization_data(const std::string &filename) {    
    std::cout<<"write_output_utilization_data:"<<filename<<std::endl;
    design::Design &design = design::Design::get_instance();    
    std::ofstream file(filename);
    std::vector<double> die_boundaries = design.get_die_boundaries();
    file << "Die "<<(int)die_boundaries.at(0)<<" "<<(int)die_boundaries.at(1)<<" "<<(int)die_boundaries.at(2)<<" "<<(int)die_boundaries.at(3)<<std::endl;    
    const legalizer::UtilizationCalculator &utilization = legalizer::UtilizationCalculator::get_instance();
    const std::vector<std::vector<design::Bin>> &bins = utilization.get_bins();
    for(const auto &row : bins){
        for(const auto &bin : row){
            file << "Bin "<<bin.get_x()<<" "<<bin.get_y()<<" "<<bin.get_rx()<<" "<<bin.get_ry()<<" "<<bin.get_utilization()<<std::endl;
        }
    }
    file.close();
}


}