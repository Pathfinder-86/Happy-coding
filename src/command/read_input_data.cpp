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
#include "../timer/timer.h"
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
    std::unordered_map<int,double> init_pin_slack_map;
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
            int pin_connection_type = (token == "NumInput")? 1 : 0;
            // NOTE!  input port is driver pin of a net, output port is sink pin of a net
            // so in connection type, input port is "output", output port is "input"
            ss >> port_num;
            for(int i = 0; i < port_num; i++){                
                std::string name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> name >> x >> y;
                circuit::Pin pin;
                pin.set_x(x);
                pin.set_y(y);
                // 0: input, 1: output, 2: clk,other                
                pin.set_pin_connection_type(pin_connection_type);
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

                // Init cell with libcell information
                const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_name);                
                circuit::Cell cell(x,y,lib_cell.get_width(),lib_cell.get_height());      
                int lib_cell_id = lib_cell.get_id();
                cell.set_lib_cell_id(lib_cell_id);                
                // only flip-flop is moveable
                if(lib_cell.is_sequential() == true){
                    cell.set_sequential(true);
                }else{
                    cell.set_sequential(false);
                }

                // handle pins
                //std::cout<<"Try get_pins_name"<<std::endl;                
                const std::vector<std::string> &pins_name = lib_cell.get_pins_name();
                const std::vector<std::pair<double, double>> &pins_position = lib_cell.get_pins_position();                
                int pin_size = pins_name.size();                                
                for(int j = 0; j < pin_size; j++){
                    
                    circuit::Pin pin;

                    // set pin position
                    pin.set_x(pins_position.at(j).first + x);                    
                    pin.set_y(pins_position.at(j).second + y);                    
                    // offset: means pin position relative to cell position
                    pin.set_offset_x(pins_position.at(j).first);
                    pin.set_offset_y(pins_position.at(j).second);

                    // set pin connection type
                    // 0: input, 1: output, 2: clk,other
                    if(lib_cell.is_sequential() == true){
                        if(pins_name.at(j).at(0) == 'D'){                    
                            pin.set_pin_connection_type(0);
                        }else if(pins_name.at(j).at(0) == 'Q'){
                            pin.set_pin_connection_type(1);
                        }else{
                            pin.set_pin_connection_type(2);
                        }
                    }else{
                        if(pins_name.at(j).at(0) == 'I'){ // IN              
                            pin.set_pin_connection_type(0);
                        }else if(pins_name.at(j).at(0) == 'O'){ // OUT
                            pin.set_pin_connection_type(1);
                        }else{
                            pin.set_pin_connection_type(2);
                        }
                    }

                    std::cout<<"add pin "<<name + "/" + pins_name.at(j)<<std::endl;
                    netlist.add_pin(pin,name + "/" + pins_name.at(j));
                    int pid = pin.get_id();
                    // set pin_id on cell
                    if(pin.is_input() == true){
                        cell.add_input_pin_id(pid);
                    }else if(pin.is_output() == true){
                        cell.add_output_pin_id(pid);
                    }else{
                        cell.add_other_pin_id(pid);
                    }
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
            netlist.set_original_pin_names();
            std::cout<<"add NumInstances finish"<<std::endl;
        }else if(token == "NumNets"){

            // determine pin slack related
            int net_num = 0;
            ss >> net_num;
            for(int i = 0; i < net_num; i++){
                std::string name;
                int pin_num = 0;
                ss >> token >> name >> pin_num;
                circuit::Net net;
                
                // add pins to net
                for(int j = 0; j < pin_num; j++){                    
                    std::string pin_name;
                    ss >> token >> pin_name;                    
                    int pin_id = netlist.get_pin_id(pin_name);
                    net.add_pin_id(pin_id);                    
                }
    
                // determine net type
                // if drive of net is FF, all pins of net are slack related --> net_slack_type = 0
                // if sink of net is FF, dirver of net is slack related --> net_slack_type = 1
                // if no FF in both source and sinks, all pins of net are not slack related --> net_slack_type = 2
                int net_slack_type = 2;
                for(int j = 0; j < pin_num ; j++){
                    // first index is driver of net
                    int pin_id = net.get_pins_id().at(j);
                    circuit::Pin &pin = netlist.get_mutable_pin(pin_id);
                    if(pin.is_port() == true){
                        continue;
                    }

                    int cell_id = pin.get_cell_id();
                    const circuit::Cell &cell = netlist.get_cell(cell_id);
                    int lib_cell_id = cell.get_lib_cell_id();
                    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
                                        
                    if(lib_cell.is_sequential() == true){
                        if(j==0){ // driver of net
                            net_slack_type = 0;
                            break;
                        }
                        // sink of net
                        pin.set_slack_related(true);
                        net_slack_type = 1;
                    }                    
                }

                if(net_slack_type == 0){
                    for(int j = 0; j < pin_num ; j++){
                        circuit::Pin &pin = netlist.get_mutable_pin(net.get_pins_id().at(j));
                        pin.set_slack_related(true);
                    }
                }else if(net_slack_type == 1){
                    circuit::Pin &pin = netlist.get_mutable_pin(net.get_pins_id().at(0));
                    pin.set_slack_related(true);
                }
                
                net.set_net_slack_type(net_slack_type);

                // add net to netlist                
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
            // TODO: add slack to timing graph
            init_pin_slack_map[pin_id] = slack;            
        }else if(token == "GatePower"){
            std::string lib_cell_name;
            double power = 0.0;
            ss >> lib_cell_name >> power;
            design.set_gate_power(lib_cell_name,power);            
        }else {
            throw std::runtime_error("Invalid token " + token);
        }        
    }
    // add nets to timing graph 
    std::cout<<"add nets to timing graph"<<std::endl;
    for(const auto &net : netlist.get_nets()){                
        int driver_pin_id = net.get_driver_pin_id();
        const circuit::Pin &driver_pin = netlist.get_pin(driver_pin_id);
        if(driver_pin.is_other() == true){
            const std::string &pin_name = netlist.get_pin_name(driver_pin_id);
            std::cout<<"skip net "<<pin_name<<std::endl;
            continue;
        }
        int net_id = net.get_id();
        const std::string &net_name = netlist.get_net_name(net_id);
        std::cout<<"add net "<<net_name<<std::endl;

        timer.add_net_into_timing_graph(net);
    }
    // add cells (qPin delay) to timing graph
    for(const auto &cell : netlist.get_cells()){        
        timer.add_cell_delay_into_timing_graph(cell);
    }

    // init timing
    std::cout<<"init timing"<<std::endl;
    timer.init_timing(init_pin_slack_map);
    // calculate each cell worst slack
    for(auto &cell : netlist.get_mutable_cells()){
        cell.calculate_slack();
    }
    // check slack
    for(const auto &cell : netlist.get_cells()){
        std::cout<<cell.get_slack()<<std::endl;
    }

    // calculate init cost
    design.calculate_cost();
    std::cout<<"init total cost:"<<design.get_total_cost()<<" timing cost:"<<design.get_timing_cost()<<" power cost:"<<design.get_power_cost()<<" area cost:"<<design.get_area_cost()<<std::endl;

    std::cout<<"read data from input done"<<std::endl;
    const config::ConfigManager &config = config::ConfigManager::get_instance();
    if(std::get<bool>(config.get_config_value("check_input_data")) == true){
        check_input_data();
    }
}



}