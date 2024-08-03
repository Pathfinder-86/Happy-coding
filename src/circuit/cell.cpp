#include "cell.h"
#include "netlist.h"
#include "../design/design.h"
#include "../design/libcell.h"
namespace circuit {

void Cell::move(int x, int y , bool update_timing){ 
    this->x = x;
    this->y = y;
    Netlist &netlist =  Netlist::get_instance();
    for(auto pin_id : get_other_pins_id()){
        Pin &pin = netlist.get_mutable_pin(pin_id);
        pin.set_x(pin.get_offset_x() + x);
        pin.set_y(pin.get_offset_y() + y);
    }
    for(auto pin_id : get_input_pins_id()){
        Pin &pin = netlist.get_mutable_pin(pin_id);
        pin.set_x(pin.get_offset_x() + x);
        pin.set_y(pin.get_offset_y() + y);
    }
    for(auto pin_id : get_output_pins_id()){
        Pin &pin = netlist.get_mutable_pin(pin_id);
        pin.set_x(pin.get_offset_x() + x);
        pin.set_y(pin.get_offset_y() + y);
    }
    
    // update_timing?
    if(update_timing){
        // TODO: update timing
    }else{
        // mark ??
    }
    
}

double Cell::get_delay() const{
    const design::Design &design = design::Design::get_instance();
    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
    return lib_cell.get_delay();
}

}