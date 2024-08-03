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
#include "../config/config_manager.h"
namespace command{

void check_input_data(){
    const design::Design &design = design::Design::get_instance();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(const auto &cell : netlist.get_cells()){
        int lib_cell_id = cell.get_lib_cell_id();
        const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);        
        if(cell.get_pins_id().size() != lib_cell.get_pins_name().size()){
            throw std::runtime_error("Cell " + std::to_string(cell.get_id()) + " has wrong number of pins");
        }    
        for(auto pin_id : cell.get_pins_id()){
            const circuit::Pin &pin = netlist.get_pin(pin_id);
            if(pin.get_cell_id() != cell.get_id()){
                throw std::runtime_error("Pin " + std::to_string(pin.get_id()) + " has wrong cell id");
            }
        }
    }
    for(const auto &net : netlist.get_nets()){        
        for(auto pin_id : net.get_pins_id()){
            const circuit::Pin &pin = netlist.get_pin(pin_id);
            if(pin.get_net_id() != net.get_id()){
                throw std::runtime_error("Pin " + std::to_string(pin.get_id()) + " has wrong net id");
            }
        }
    }
}

void CommandManager::read_input_data(const std::string &filename) {    
    std::cout<<"read data from input "<<filename<<std::endl;
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    design::Design &design = design::Design::get_instance();
    timer::Timer &timer = timer::Timer::get_instance();
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file " + filename);
    }else{
        std::cout<<"open file "<<filename<<std::endl;
    }
    std::string token;
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    std::cout << "Parsing input data..." << std::endl;
    while( ss >> token){
        if(token == "Alpha"){
            double factor = 0.0;
            ss >> factor;
            design.set_timing_factor(factor);            
            std::cout<<"set timing factor finish"<<std::endl;
        }else if(token == "Beta"){
            double factor = 0.0;
            ss >> factor;
            design.set_power_factor(factor);
            std::cout<<"set power factor finish"<<std::endl;
        }else if(token == "Gamma"){
            double factor = 0.0;
            ss >> factor;
            design.set_area_factor(factor);
            std::cout<<"set area factor finish"<<std::endl;
        }else if(token == "Lambda"){
            double factor = 0.0;
            ss >> factor;
            design.set_utilization_factor(factor);
            std::cout<<"set utilization factor finish"<<std::endl;
        }else if(token == "DieSize"){
            for(int i = 0; i < 4; i++){
                double boundary = 0.0;
                ss >> boundary;
                design.add_die_boundary(boundary);
            }
            std::cout<<"set die boundary finish"<<std::endl;
        }else if(token == "NumInput" || token == "NumOutput"){
            int port_num = 0;
            ss >> port_num;
            for(int i = 0; i < port_num; i++){                
                std::string name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> name >> x >> y;
                circuit::Pin pin;
                pin.set_x(x);
                pin.set_y(y);
                netlist.add_pin(pin,name);   
            }

            std::cout<<"add port finish"<<std::endl;

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
                if(pin_name.at(0) == 'D'){                    
                    libcell.add_pin(pin_name,x,y,0);
                }else if(pin_name.at(0) == 'Q'){
                    libcell.add_pin(pin_name,x,y,1);
                }else{
                    libcell.add_pin(pin_name,x,y,2);
                }
            }
            libcell.set_sequential(true);
            libcell.set_bits(bits);
            design.add_lib_cell(libcell);
            int lib_cell_id = libcell.get_id();
            design.add_flipflop_id_to_bits_group(bits,lib_cell_id);
            std::cout<<"add FF finish"<<std::endl;            
        }else if(token == "Gate"){
            std::string name;
            int pin_num = 0;
            double width = 0.0, height = 0.0;
            ss >> name >> width >> height >> pin_num;
            std::cout<<"Gate "<<name<<" "<<width<<" "<<height<<" "<<pin_num<<std::endl;
            design::LibCell libcell(name,width,height);

            for(int i = 0; i < pin_num; i++){
                std::string pin_name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> pin_name >> x >> y;                                
                libcell.add_pin(pin_name,x,y,2);
            }
            libcell.set_sequential(false);
            design.add_lib_cell(libcell);
            std::cout<<"add Gate finish"<<std::endl;                        
        } else if(token == "NumInstances"){
            int cell_num = 0;
            ss >> cell_num;
            for(int i = 0; i < cell_num; i++){
                std::string name;
                std::string lib_cell_name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> name >> lib_cell_name >> x >> y;
                std::cout<<"Instance "<<name<<" "<<lib_cell_name<<" "<<x<<" "<<y<<std::endl;
                // init cell
                const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_name);                
                circuit::Cell cell(x,y,lib_cell.get_width(),lib_cell.get_height());        
                int lib_cell_id = lib_cell.get_id();
                cell.set_lib_cell_id(lib_cell_id);
                // handle pins
                //std::cout<<"Try get_pins_name"<<std::endl;                
                const std::vector<std::string> &pins_name = lib_cell.get_pins_name();
                const std::vector<std::pair<double, double>> &pins_position = lib_cell.get_pins_position();                
                int pin_size = pins_name.size();
                //for(int j=0;j<pin_size;j++){
                //    std::cout<<pins_name.at(j)<<std::endl;
                //    std::cout<<pins_position.at(j).first<<" "<<pins_position.at(j).second<<std::endl;
                //}
                //std::cout<<"pins_name size "<<pin_size<<std::endl;           
                for(int j = 0; j < pin_size; j++){
                    //std::cout<<"loop:"<<j<<std::endl;
                    circuit::Pin pin;
                    pin.set_x(pins_position.at(j).first + x);                    
                    pin.set_y(pins_position.at(j).second + y);
                    pin.set_offset_x(pins_position.at(j).first);
                    pin.set_offset_y(pins_position.at(j).second);
                    std::cout<<"add pin "<<name + "/" + pins_name.at(j)<<std::endl;
                    netlist.add_pin(pin,name + "/" + pins_name.at(j));
                    int pid = pin.get_id();
                    // set pin_id on cell
                    cell.add_pin_id(pid);
                }
                //std::cout<<"add pins finish"<<std::endl;
                netlist.add_cell(cell,name);
                // set cell_id on pin
                int cell_id = cell.get_id();
                for(auto pin_id : cell.get_pins_id()){
                    circuit::Pin &pin = netlist.get_mutable_pin(pin_id);
                    pin.set_cell_id(cell_id);
                }
            }
            std::cout<<"add NumInstances finish"<<std::endl;
        }else if(token == "NumNets"){
            int net_num = 0;
            ss >> net_num;
            for(int i = 0; i < net_num; i++){
                std::string name;
                int pin_num = 0;
                ss >> token >> name >> pin_num;
                circuit::Net net;
                for(int j = 0; j < pin_num; j++){                    
                    std::string pin_name;
                    ss >> token >> pin_name;                    
                    int pin_id = netlist.get_pin_id(pin_name);
                    net.add_pin_id(pin_id);
                }
                netlist.add_net(net,name);
                int net_id = net.get_id();
                for(auto pin_id : net.get_pins_id()){
                    circuit::Pin &pin = netlist.get_mutable_pin(pin_id);
                    pin.set_net_id(net_id);
                }
            }
            std::cout<<"add NumNets finish"<<std::endl;            
        }else if(token == "BinWidth"){
            double x = 0.0;
            double y = 0.0;
            ss >> x;
            ss >> token >> y;
            design.set_bin_size(x,y);
            double bin_max_utilization = 0.0;
            ss >> token >> bin_max_utilization;
            design.set_bin_max_utilization(bin_max_utilization);
            design.init_bins();
        }else if(token == "PlacementRows"){
            double x = 0.0, y = 0.0;
            double site_width = 0.0, site_height = 0.0;
            int site_num = 0;
            ss >> x >> y >> site_width >> site_height >> site_num;
            double width = site_width * site_num;
            design.add_row(x,y,width,site_height,site_width);
        }else if(token == "DisplacementDelay"){
            double delay = 0.0;
            ss >> delay;
            design.set_displacement_delay(delay);
        }else if(token == "QpinDelay"){
            std::string lib_cell_name;
            double delay = 0.0;
            ss >> lib_cell_name >> delay;
            design.set_qpin_delay(lib_cell_name,delay);            
        }else if(token == "TimingSlack"){
            std::string cell_name, pin_name;
            double slack = 0.0;
            ss >> cell_name >> pin_name >> slack;
            int pin_id = netlist.get_pin_id(cell_name + "/" + pin_name);
            circuit::Pin &pin = netlist.get_mutable_pin(pin_id);
            // TODO: add timing node to timing graph
        }else if(token == "GatePower"){
            std::string lib_cell_name;
            double power = 0.0;
            ss >> lib_cell_name >> power;
            design.set_gate_power(lib_cell_name,power);            
        }else {
            throw std::runtime_error("Invalid token " + token);
        }

        timer.update_timing();
    }
    std::cout<<"read data from input done"<<std::endl;
    const config::ConfigManager &config = config::ConfigManager::get_instance();
    if(std::get<bool>(config.get_config_value("check_input_data")) == true){
        check_input_data();
    }
}



}