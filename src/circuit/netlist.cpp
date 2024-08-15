#include "netlist.h"
#include <iostream>
#include <../timer/timer.h>
namespace circuit {

bool Netlist::cluster_cells(int id1, int id2){
    const design::Design &design = design::Design::get_instance();    
    if(id1 == id2){
        std::cout<<" Error: Cannot cluster the same cell\n";
        return false;
    }
    if(id1 > id2) {
        std::swap(id1, id2);
    }
    Cell& cell1 = get_mutable_cell(id1);
    Cell& cell2 = get_mutable_cell(id2);
    if(cell1.get_parent() != -1 || cell2.get_parent() != -1){
        std::cout<<" Error: Cannot cluster a cell that is already clustered" << id1 << " " << id2 << std::endl;
        return false;
    }

    // TODO:merge cell2 into cell1
    // get cell1 and cell2 origin lib_cell
    int lib_cell_id1 = cell1.get_lib_cell_id();
    int lib_cell_id2 = cell2.get_lib_cell_id();
    const design::LibCell &lib_cell1 = design.get_lib_cell(lib_cell_id1);
    const design::LibCell &lib_cell2 = design.get_lib_cell(lib_cell_id2);
    int new_bits = lib_cell1.get_bits() + lib_cell2.get_bits();
    // Try get new bits lib_cells target
    const std::vector<int> &bit_lib_cells_id = design.get_bit_flipflops_id(new_bits);
    if(bit_lib_cells_id.empty()){
        std::cout<<" Error: Cannot find a flip-flop with " << new_bits << " bits\n";
        return false;
    }
    // MUST pass previous checks and call modify_circuit_since_merge_cell
    modify_circuit_since_merge_cell(id1, id2);  
    return true;
}

void Netlist::best_libcell_index(const std::vector<int> &bit_lib_cells_id, int &good_index){
    return;
}

void Netlist::modify_circuit_since_merge_cell(int id1, int id2){
    const std::string &cell_name1 = get_cell_name(id1);
    const std::string &cell_name2 = get_cell_name(id2);
    std::cout<<"modify_circuit_since_merge_cell: "<<cell_name1<<" "<<cell_name2<<std::endl;
    const design::Design &design = design::Design::get_instance();
    Cell& cell1 = get_mutable_cell(id1);
    Cell& cell2 = get_mutable_cell(id2);
    int lib_cell_id1 = cell1.get_lib_cell_id();
    int lib_cell_id2 = cell2.get_lib_cell_id();
    const design::LibCell &lib_cell1 = design.get_lib_cell(lib_cell_id1);
    const design::LibCell &lib_cell2 = design.get_lib_cell(lib_cell_id2);
    // netlist.get_cell_name()    
    int new_bits = lib_cell1.get_bits() + lib_cell2.get_bits();
    // Asummption: get first flip-flop in the list unit compare function finish
    // TODO: Pick the best flip-flop
    int good_index = 0;
    const std::vector<int> &bit_lib_cells_id = design.get_bit_flipflops_id(new_bits);
    best_libcell_index(bit_lib_cells_id, good_index);
    int new_lib_cell_id = bit_lib_cells_id.at(good_index);

    // set cell1 to new_lib_cell
    cell1.set_lib_cell_id(new_lib_cell_id);


    // cell1 new location 
    // TODO: aummption: center of cell1 and cell2
    int new_cell_x = (cell1.get_x() + cell2.get_x()) / 2;
    int new_cell_y = (cell1.get_y() + cell2.get_y()) / 2;
    cell1.set_x(new_cell_x);
    cell1.set_y(new_cell_y);
    // change cell1 width and height
    const design::LibCell &new_lib_cell = design.get_lib_cell(new_lib_cell_id);
    std::cout<<"new_lib_cell: "<<new_lib_cell.get_name()<<std::endl;
    cell1.set_w(new_lib_cell.get_width());
    cell1.set_h(new_lib_cell.get_height());
    
    // set cell2 parent to cell1
    cell2.set_parent(id1);
    
    // handle pins mapping
    // need to update pin name, pin location from lib_cell
    // input
    std::vector<std::string> new_lib_cell_input_pins_name = new_lib_cell.get_input_pins_name();
    std::vector<std::pair<double, double>> new_lib_cell_input_pins_position = new_lib_cell.get_input_pins_position();
    std::vector<int> cell1_input_pins_id = cell1.get_input_pins_id();
    std::vector<int> cell2_input_pins_id = cell2.get_input_pins_id();
    std::vector<int> new_cell_input_pins_id;
    new_cell_input_pins_id.reserve(cell1_input_pins_id.size() + cell2_input_pins_id.size());
    new_cell_input_pins_id.insert(new_cell_input_pins_id.end(), cell1_input_pins_id.begin(), cell1_input_pins_id.end());   
    new_cell_input_pins_id.insert(new_cell_input_pins_id.end(), cell2_input_pins_id.begin(), cell2_input_pins_id.end());
    std::cout<<"new_cell_input_pins_id check "<<new_cell_input_pins_id.size()<<std::endl;
    for(int i = 0; i < static_cast<int>(new_cell_input_pins_id.size()); i++){
        int pin_id = new_cell_input_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        // netlist.update_pin_name()
        const std::string &new_pin_name = cell_name1 + "/" + new_lib_cell_input_pins_name.at(i);
        std::cout<<"new_pin_name: "<<new_pin_name<<std::endl;
        update_pin_name(pin_id, new_pin_name);
        pin.set_offset_x(new_lib_cell_input_pins_position.at(i).first);
        pin.set_offset_y(new_lib_cell_input_pins_position.at(i).second);
        pin.set_x(new_cell_x + new_lib_cell_input_pins_position.at(i).first);
        pin.set_y(new_cell_y + new_lib_cell_input_pins_position.at(i).second);
        pin.set_cell_id(id1);
    }
    std::cout<<"new_cell_input_pins_id: "<<new_cell_input_pins_id.size()<<std::endl;    
    // output
    std::vector<std::string> new_lib_cell_output_pins_name = new_lib_cell.get_output_pins_name();
    std::vector<std::pair<double, double>> new_lib_cell_output_pins_position = new_lib_cell.get_output_pins_position();
    std::vector<int> cell1_output_pins_id = cell1.get_output_pins_id();
    std::vector<int> cell2_output_pins_id = cell2.get_output_pins_id();    
    std::vector<int> new_cell_output_pins_id;
    new_cell_output_pins_id.reserve(cell1_output_pins_id.size() + cell2_output_pins_id.size());
    new_cell_output_pins_id.insert(new_cell_output_pins_id.end(), cell1_output_pins_id.begin(), cell1_output_pins_id.end());
    new_cell_output_pins_id.insert(new_cell_output_pins_id.end(), cell2_output_pins_id.begin(), cell2_output_pins_id.end());
    std::cout<<"new_cell_output_pins_id check "<<new_cell_output_pins_id.size()<<std::endl;
    std::cout<<"cell1_output_pins_id check "<<cell1_output_pins_id.size()<<std::endl;
    std::cout<<"cell2_output_pins_id check "<<cell2_output_pins_id.size()<<std::endl;
    for(int pid : new_cell_output_pins_id){
        std::string pin_name = get_pin_name(pid);
        std::cout<<"new_cell_output_pins_id: "<<pin_name<<std::endl;
    }
    for(int i = 0; i < static_cast<int>(new_cell_output_pins_id.size()); i++){
        int pin_id = new_cell_output_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        // netlist.update_pin_name()
        const std::string &new_pin_name = cell_name1 + "/" + new_lib_cell_output_pins_name.at(i);
        std::cout<<"new_pin_name: "<<new_pin_name<<std::endl;
        update_pin_name(pin_id,  new_pin_name);        
        pin.set_offset_x(new_lib_cell_output_pins_position.at(i).first);
        pin.set_offset_y(new_lib_cell_output_pins_position.at(i).second);
        pin.set_x(new_cell_x + new_lib_cell_output_pins_position.at(i).first);
        pin.set_y(new_cell_y + new_lib_cell_output_pins_position.at(i).second);
        pin.set_cell_id(id1);
        std::cout<<"new_pin_name: "<<new_pin_name<<" finish"<<std::endl;
    }
    std::cout<<"new_cell_output_pins_id: "<<new_cell_output_pins_id.size()<<std::endl;
    // other
    std::vector<std::string> new_lib_cell_other_pins_name = new_lib_cell.get_other_pins_name();
    std::vector<std::pair<double, double>> new_lib_cell_other_pins_position = new_lib_cell.get_other_pins_position();
    std::vector<int> cell1_other_pins_id = cell1.get_other_pins_id();
    std::vector<int> cell2_other_pins_id = cell2.get_other_pins_id();
    std::vector<int> new_cell_other_pins_id;
    new_cell_other_pins_id.reserve(cell1_other_pins_id.size() + cell2_other_pins_id.size());
    new_cell_other_pins_id.insert(new_cell_other_pins_id.end(), cell1_other_pins_id.begin(), cell1_other_pins_id.end());
    new_cell_other_pins_id.insert(new_cell_other_pins_id.end(), cell2_other_pins_id.begin(), cell2_other_pins_id.end());
    
    const std::string &new_libcell_other_pin_name = new_lib_cell_other_pins_name.at(0);
    const std::pair<double, double> &new_libcell_other_pin_position = new_lib_cell_other_pins_position.at(0);

    for(int i = 0; i < static_cast<int>(new_cell_other_pins_id.size()); i++){
        int pin_id = new_cell_other_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        // netlist.update_pin_name()
        update_pin_name(pin_id,  cell_name1 + "/" + new_libcell_other_pin_name);
        pin.set_offset_x(new_libcell_other_pin_position.first);
        pin.set_offset_y(new_libcell_other_pin_position.second);
        pin.set_x(new_cell_x + new_libcell_other_pin_position.first);
        pin.set_y(new_cell_y + new_libcell_other_pin_position.second);
        pin.set_cell_id(id1);
    }
    std::cout<<"new_cell_other_pins_id: "<<new_cell_other_pins_id.size()<<std::endl;
    // update cell1 pins
    cell1.set_input_pins_id(new_cell_input_pins_id);
    cell1.set_output_pins_id(new_cell_output_pins_id);
    cell1.set_other_pins_id(new_cell_other_pins_id);
    // clear cell2 pins
    cell2.clear();


    // update timing information
    // cell1 q_pin_delay change
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing(id1);
}

bool Netlist::check_overlap(){
    for(int i = 0; i < static_cast<int>(cells.size()); i++){
        if(cells.at(i).get_parent() != -1){
                continue;
        }

        for(int j = i + 1; j < static_cast<int>(cells.size()); j++){
            if(cells.at(j).get_parent() != -1){
                continue;
            }
            if(cells.at(i).overlap(cells.at(j))){
                return true;
            }
        }
    }
}

bool Netlist::check_out_of_die(){
    const design::Design &design = design::Design::get_instance();
    std::vector<double> die_boundaries = design.get_die_boundaries();
    std::cout<<"die_boundaries: ";
    for(double boundary: die_boundaries){
        std::cout<<boundary<<" ";
    }
    std::cout<<std::endl;

    for(auto &cell: cells){
        if(cell.get_parent() != -1){
            continue;
        }
        if(cell.get_x() < die_boundaries.at(0) || cell.get_rx() > die_boundaries.at(2)){
            print_cell_info(cell);
            return true;
        }
        if(cell.get_y() < die_boundaries.at(1) || cell.get_ry() > die_boundaries.at(3)){
            print_cell_info(cell);
            return true;
        }
    }
    return false;
}

void Netlist::print_cell_info(const Cell &cell){
    const design::Design &design = design::Design::get_instance();
    const design::LibCell &lib_cell = design.get_lib_cell(cell.get_lib_cell_id());
    std::cout<<"Cell: "<<get_cell_name(cell.get_id())<<" LibCell: "<<lib_cell.get_name()<<" x: "<<cell.get_x()<<" y: "<<cell.get_y()<<" rx: "<<cell.get_rx()<<" ry: "<<cell.get_ry()<<std::endl;

}
} // namespace circuit