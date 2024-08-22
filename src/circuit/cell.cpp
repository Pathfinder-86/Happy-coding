#include "cell.h"
#include "netlist.h"
#include "../design/design.h"
#include "../design/libcell.h"
#include "../timer/timer.h"
namespace circuit {

void Cell::move(int x, int y){ 
    this->x = x;
    this->y = y;
    Netlist &netlist =  Netlist::get_instance();
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
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing( get_id() );
}

double Cell::get_delay() const{
    const design::Design &design = design::Design::get_instance();
    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
    return lib_cell.get_delay();
}

double Cell::get_power() const {
    const design::Design &design = design::Design::get_instance();
    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
    return lib_cell.get_power();
}

void Cell::calculate_slack(){
    timer::Timer &timer = timer::Timer::get_instance();    
    double worst_slack = 0.0;
    for(int pid : input_pins_id){
        worst_slack = std::min(worst_slack,timer.get_slack(pid));
    }
    set_slack(worst_slack);
}

bool Cell::overlap(const Cell &cell) const {
    if(cell.get_rx() <= x || cell.get_x() >= get_rx()){
        return false;
    }
    if(cell.get_ry() <= y || cell.get_y() >= get_ry()){
        return false;
    }
    return true;

}
}