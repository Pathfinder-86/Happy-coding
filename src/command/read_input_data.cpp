#include "command_manager.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../design/design.h"
#include "../design/libcell.h"
#include "../circuit/pin.h"
#include "../circuit/netlist.h"
#include "../circuit/net.h"
#include "../circuit/cell.h"

namespace command{
void CommandManager::read_input_data(const std::string &filename) {    
    std::cout<<"read data from input"<<filename<<std::endl;
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    design::Design &design = design::Design::get_instance();
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file " + filename);
    }
    std::string token;
    std::stringstream ss;
    ss << file.rdbuf();
    while( ss >> token){
        if(token == "Alpha"){
            double factor = 0.0;
            ss >> factor;
            design.set_timing_factor(factor);
        }else if(token == "Beta"){
            double factor = 0.0;
            ss >> factor;
            design.set_power_factor(factor);
        }else if(token == "Gamma"){
            double factor = 0.0;
            ss >> factor;
            design.set_area_factor(factor);
        }else if(token == "Lambda"){
            double factor = 0.0;
            ss >> factor;
            design.set_utilization_factor(factor);
        }else if(token == "DieSize"){
            for(int i = 0; i < 4; i++){
                double boundary = 0.0;
                ss >> boundary;
                design.add_die_boundary(boundary);
            }
        }else if(token == "NumInput" || token == "NumOutput"){
            int port_num = 0;
            ss >> port_num;
            for(int i = 0; i < port_num; i++){                
                std::string name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> name >> x >> y;
                circuit::Pin pin;
                netlist.add_pin(pin,name);                            
            }
        }else if(token == "FlipFlop"){
            int bits = 0,pin_num = 0;
            std::string name;
            double width = 0.0, height = 0.0;
            ss >> bits >> name >> width >> height >> pin_num;
            design::LibCell libcell(name,width,height);

            for(int i = 0; i < pin_num; i++){
                std::string pin_name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> pin_name >> x >> y;                                
                libcell.add_pin(pin_name,x,y);
            }
            libcell.set_sequential(true);
            design.add_lib_cell(libcell);
            int lib_cell_id = libcell.get_id();
            design.add_flipflop_id(bits,lib_cell_id);
        }else if(token == "Gate"){
            std::string name;
            int pin_num = 0;
            double width = 0.0, height = 0.0;
            ss >> name >> width >> height >> pin_num;
            design::LibCell libcell(name,width,height);

            for(int i = 0; i < pin_num; i++){
                std::string pin_name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> pin_name >> x >> y;                                
                libcell.add_pin(pin_name,x,y);
            }
            libcell.set_sequential(false);
            design.add_lib_cell(libcell);                        
        } else if(token == "NumInstances"){

        }

    }
}
}