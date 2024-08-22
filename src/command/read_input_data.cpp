#include "command_manager.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../design/design.h"
#include "../design/libcell.h"
#include "../circuit/pin.h"
#include "../circuit/original_netlist.h"
#include "../circuit/net.h"
#include "../circuit/cell.h"
#include "../config/config_manager.h"
#include "../timer/timer.h"
#include "../estimator/solution.h"
#include "../estimator/cost.h"
#include "../legalizer/utilization.h"
#include "../legalizer/legalizer.h"
#include "../runtime/runtime.h"
#include "../estimator/lib_cell_evaluator.h"
#include "../circuit/original_netlist.h"
namespace command{

void check_input_data(){
}

void CommandManager::read_input_data(const std::string &filename) {    
    std::cout<<"PARSE:: read data from input "<<filename<<std::endl;
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    design::Design &design = design::Design::get_instance();
    timer::Timer &timer = timer::Timer::get_instance();
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    const runtime::RuntimeManager& runtime_manager = runtime::RuntimeManager::get_instance();

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
    std::cout << "PARSE:: INIT" << std::endl;
    std::unordered_map<int,double> init_pin_slack_map;
    while( ss >> token){
        if(token == "Alpha"){
            double factor = 0.0;
            ss >> factor;
            design.set_timing_factor(factor);            
            //std::cout<<"set timing factor finish"<<std::endl;
        }else if(token == "Beta"){
            double factor = 0.0;
            ss >> factor;
            design.set_power_factor(factor);
            //std::cout<<"set power factor finish"<<std::endl;
        }else if(token == "Gamma"){
            double factor = 0.0;
            ss >> factor;
            design.set_area_factor(factor);
            //std::cout<<"set area factor finish"<<std::endl;
        }else if(token == "Lambda"){
            double factor = 0.0;
            ss >> factor;
            design.set_utilization_factor(factor);
            //std::cout<<"set utilization factor finish"<<std::endl;
        }else if(token == "DieSize"){
            for(int i = 0; i < 4; i++){
                double boundary = 0.0;
                ss >> boundary;
                design.add_die_boundary(boundary);
            }
            //std::cout<<"set die boundary finish"<<std::endl;
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
                original_netlist.add_pin(pin,name);
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
                libcell.add_pin(pin_name,x,y,2);
            }
            libcell.set_sequential(false);
            design.add_lib_cell(libcell);                
        } else if(token == "NumInstances"){
            int cell_num = 0;
            ss >> cell_num;
            for(int i = 0; i < cell_num; i++){
                std::string name;
                std::string lib_cell_name;
                double x = 0.0;
                double y = 0.0;
                ss >> token >> name >> lib_cell_name >> x >> y;
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
                            pin.set_ff_pin(true);               
                        }else if(pins_name.at(j).at(0) == 'Q'){
                            pin.set_pin_connection_type(1);
                            pin.set_ff_pin(true);                            
                        }else{
                            pin.set_pin_connection_type(2);                            
                            pin.set_ff_pin(false);
                        }
                    }else{
                        pin.set_ff_pin(false);
                        if(pins_name.at(j).at(0) == 'I'){ // IN              
                            pin.set_pin_connection_type(0);
                        }else if(pins_name.at(j).at(0) == 'O'){ // OUT
                            pin.set_pin_connection_type(1);
                        }else{
                            pin.set_pin_connection_type(2);
                        }
                    }
                    original_netlist.add_pin(pin,name + "/" + pins_name.at(j));
                    int pid = pin.get_id();
                    // set pin_id on cell
                    if(pin.is_input() == true){
                        cell.add_input_pin_id(pid);
                    }else if(pin.is_output() == true){
                        cell.add_output_pin_id(pid);
                    }else{
                        cell.set_other_pin_id(pid);
                    }
                }

                original_netlist.add_cell(cell,name);
                // set cell_id on pin
                int cell_id = cell.get_id();
                for(auto pin_id : cell.get_pins_id()){
                    circuit::Pin &pin = original_netlist.get_mutable_pin(pin_id);
                    pin.set_cell_id(cell_id);
                }
            }
            std::cout<<"PARSE:: MAPPING"<<std::endl;
            // MAPPING 
            // Add ff cells from original netlist to netlist
            const std::vector<circuit::Cell> &cells = original_netlist.get_cells();            
            for(const auto &cell : cells){                
                if(cell.is_sequential() == true){
                    circuit::Cell ff_cell = cell;
                    netlist.add_cell(ff_cell);
                    int ff_cell_id = ff_cell.get_id();
                    original_netlist.add_ff_cell_id_to_original_cell_id(ff_cell_id,cell.get_id());                                   
                }
            }
            // MAPPING
            // Add ff pins from original netlist to netlist
            const std::vector<circuit::Pin> &pins = original_netlist.get_pins();
            for(const auto &pin : pins){                
                if(pin.is_ff_pin() == true){
                    circuit::Pin ff_pin = pin;
                    netlist.add_pin(ff_pin);
                    int ff_pin_id = ff_pin.get_id();                    
                    original_netlist.add_ff_pin_id_to_original_pin_id(ff_pin_id,pin.get_id());
                }
            }
            // MAPPING
            // netlist pin change cell_id and sequential_cells_id
            for(auto &cell : netlist.get_mutable_cells()){
                int cell_id = cell.get_id();
                int bits = cell.get_bits();
                std::vector<int> new_input_pins_id;
                std::vector<int> new_output_pins_id;
                new_input_pins_id.reserve(bits);
                new_output_pins_id.reserve(bits);
                for(auto pin_id : cell.get_input_pins_id()){
                    int ff_pin_id = original_netlist.get_ff_pin_id(pin_id);
                    circuit::Pin &pin = netlist.get_mutable_pin(ff_pin_id);
                    pin.set_cell_id(cell_id);
                    new_input_pins_id.push_back(ff_pin_id);                    
                }
                for(auto pin_id : cell.get_output_pins_id()){
                    int ff_pin_id = original_netlist.get_ff_pin_id(pin_id);
                    circuit::Pin &pin = netlist.get_mutable_pin(ff_pin_id);
                    pin.set_cell_id(cell_id);
                    new_output_pins_id.push_back(ff_pin_id);                    
                }
                cell.set_input_pins_id(new_input_pins_id);
                cell.set_output_pins_id(new_output_pins_id);
                netlist.add_sequential_cell(cell_id);
            }
            std::cout<<"PARSE:: MAPPING FINISH"<<std::endl;
        }else if(token == "NumNets"){

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
                    int pin_id = original_netlist.get_pin_id(pin_name);
                    net.add_pin_id(pin_id);                    
                }

                // add net to original_netlist                
                original_netlist.add_net(net,name);
                int net_id = net.get_id();
                for(auto pin_id : net.get_pins_id()){
                    circuit::Pin &pin = original_netlist.get_mutable_pin(pin_id);
                    pin.set_net_id(net_id);
                }

                const std::vector<int> &pins_id = net.get_pins_id();
                if(pins_id.size() < 2){
                    continue;
                }
                // CLK NET
                bool is_clk_net = false;
                for(int i=1;i<pin_num;i++){
                    int pin_id = net.get_pins_id().at(i);
                    const std::string &pin_name = original_netlist.get_pin_name(pins_id.at(1));
                    if(pin_name.find("CLK") != std::string::npos){
                        is_clk_net = true;
                        break;
                    }    
                }
                
                if(is_clk_net == true){
                    // MAPPING
                    std::unordered_set<int> clk_group;
                    for(int i=1;i<pin_num;i++){
                        int pin_id = net.get_pins_id().at(i);
                        const circuit::Pin &pin = original_netlist.get_pin(pin_id);
                        int cell_id = pin.get_cell_id();
                        const circuit::Cell &cell = original_netlist.get_cell(cell_id);
                        if(cell.is_sequential() == true){
                            int ff_cell_id = original_netlist.get_ff_cell_id(cell_id);
                            clk_group.insert(ff_cell_id);
                        }
                    }                    
                    if(clk_group.size() > 1){                        
                        netlist.add_clk_group(clk_group);
                    }
                }
            }            
        }else if(token == "BinWidth"){
            double x = 0.0;
            double y = 0.0;
            ss >> x;
            ss >> token >> y;
            design.set_bin_size(x,y);
            double bin_max_utilization = 0.0;
            ss >> token >> bin_max_utilization;
            design.set_bin_max_utilization(bin_max_utilization / 100. );            
        }else if(token == "PlacementRows"){
            double x,y,site_width,site_height;
            int site_num;
            ss >> x >> y >> site_width >> site_height >> site_num;            
            legalizer.add_row((int)x,(int)y,(int)site_width,(int)site_height,site_num);
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
            int pin_id = original_netlist.get_pin_id(cell_name + "/" + pin_name);            
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
    std::cout << "PARSE:: FINISH" << std::endl;    
    runtime_manager.get_runtime();
    
    std::cout<<"EVALUATOR:: LIB_INIT"<<std::endl;
    // libcell evaluator
    estimator::FFLibcellCostManager &ff_libcell_cost_manager = estimator::FFLibcellCostManager::get_instance();    
    ff_libcell_cost_manager.init();
    std::cout<<"EVALUATOR:: LIB_END"<<std::endl;
    runtime_manager.get_runtime();

    // init timing
    std::cout<<"TIMER:: INIT"<<std::endl;
    timer.init_timing(init_pin_slack_map);
    // calculate each cell worst slack
    for(auto &cell : netlist.get_mutable_cells()){
        cell.calculate_slack();
    }
    // create timing nodes and connection    
    timer.create_timing_graph();
    std::cout<<"TIMER:: FINISH"<<std::endl;
    runtime_manager.get_runtime();


    std::cout<<"LEGAL:: INIT"<<std::endl;
    // update bins
    legalizer::UtilizationCalculator &utilization_calculator = legalizer::UtilizationCalculator::get_instance();
    utilization_calculator.update_bins_utilization();        
    // check legal
    legalizer.init();
    if(legalizer.valid()){
        std::cout<<"LEGAL:: FINISH"<<std::endl;
    }else{
        std::cout<<"LEGAL:: INIT_LEGAL FAIL may need to cluster to reduce area"<<std::endl;
    }
    runtime_manager.get_runtime();


    // calculate init 
    estimator::CostCalculator &cost_calculator = estimator::CostCalculator::get_instance();
    cost_calculator.calculate_cost();    
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();    
    solution_manager.keep_init_solution();

    std::cout<<"PARSE:: read_input_data and init FINISH"<<std::endl;

}



}