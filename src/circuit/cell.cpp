#include "cell.h"
#include "netlist.h"
namespace circuit {

void Cell::move(int x, int y) {
    this->x = x;
    this->y = y;
    Netlist &netlist =  Netlist::get_instance();
    for(auto pin_id : pins_id){
        Pin &pin = netlist.get_mutable_pin(pin_id);
        pin.set_x(pin.get_offset_x() + x);
        pin.set_y(pin.get_offset_y() + y);
    }
}

}