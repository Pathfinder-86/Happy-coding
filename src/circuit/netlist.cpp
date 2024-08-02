#include "netlist.h"
#include <iostream>
namespace circuit {

/*void Netlist::cluster_cells(int id1, int id2){
    const design::Design &design = design::Design::get_instance();
    if(id1 == id2){
        std::cout<<" Error: Cannot cluster the same cell\n";
        return;
    }
    if(id1 > id2) {
        std::swap(id1, id2);
    }
    Cell& cell1 = get_mutable_cell(id1);
    Cell& cell2 = get_mutable_cell(id2);
    if(cell1.get_parent() != -1 || cell2.get_parent() != -1){
        std::cout<<" Error: Cannot cluster a cell that is already clustered" << id1 << " " << id2 << std::endl;
        return;
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
        return;
    }
    // Asummption: get first flip-flop in the list unit compare function finish
    // set cell1 to new_lib_cell
    int new_lib_cell_id = bit_lib_cells_id.at(0);
    cell1.set_lib_cell_id(new_lib_cell_id);
    // change cell1 width and height
    const design::LibCell &new_lib_cell = design.get_lib_cell(new_lib_cell_id);
    cell1.set_w(new_lib_cell.get_width());
    cell1.set_h(new_lib_cell.get_height());    
    // set cell2 parent to cell1
    cell2.set_parent(id1);
    // change all pins of cell2 to cell1
    for(auto pin_id : cell2.get_pins_id()){
        Pin& pin = get_mutable_pin(pin_id);
        pin.set_cell_id(id1);
    }
    // add all pins of cell2 to cell1
    for(auto pin_id : cell2.get_pins_id()){
        cell1.add_pin_id(pin_id);
    }
    // new pin mapping to new_libcell
    std::vector<std::pair<double,double>> pin_positions = new_lib_cell.get_pin_positions();
    
}*/

}