#include "timer.h"
#include <cmath>
#include <queue>
#include <stack>
#include <../circuit/netlist.h>
#include <../circuit/original_netlist.h>
#include <../design/design.h>
#include <../design/libcell.h>
#include <../circuit/cell.h>
#include <../circuit/pin.h>
#include <../circuit/net.h>
#include <fstream>
#include "../config/config_manager.h"
#include <sstream>
namespace timer {

void Timer::write_init_timing_data(){
    const config::ConfigManager &config_manager = config::ConfigManager::get_instance();
    int write_case = std::get<int>(config_manager.get_config_value("timer_case"));
    std::string write_case_str;
    std::string pre_fix = "timer_init/";
    if(write_case == 0){
        write_case_str = "timer_input.txt";
    }else if(write_case == 1){
        write_case_str = "timer_testcase1_0812.txt";
    }else if(write_case == 2){
        write_case_str = "timer_testcase2_0812.txt";
    }else if(write_case == 3){
        write_case_str = "timer_tiny.txt";
    }else if(write_case == 4){
        write_case_str = "timer_testcase3.txt";
    }
    const std::string &path = pre_fix + write_case_str;
    std::ofstream file(path);
    for(auto& [id,timing_node] : timing_nodes){
        double q_pin_delay = timing_node.get_q_pin_delay();
        double q_pin_output_pin_placement_delay = timing_node.get_q_pin_output_pin_placement_delay();
        double input_pin_d_pin_placement_delay = timing_node.get_input_pin_d_pin_placement_delay();
        std::unordered_set<int> affected_d_pins = timing_node.get_affected_d_pins();
        std::pair<int,int> q_pin_output_pin_location = timing_node.get_q_pin_output_pin_location();
        std::pair<int,int> d_pin_input_pin_location = timing_node.get_d_pin_input_pin_location();
        double slack = timing_node.get_slack();
        file << id << " " << q_pin_delay << " " << q_pin_output_pin_placement_delay << " " << input_pin_d_pin_placement_delay << " " << slack << " " << q_pin_output_pin_location.first << " " << q_pin_output_pin_location.second << " " << d_pin_input_pin_location.first << " " << d_pin_input_pin_location.second << " ";
        int size = affected_d_pins.size();
        file << size << " ";        
        for(int affected_d_pin : affected_d_pins){
            file << affected_d_pin << " ";
        }
        file << std::endl;
    }
}

void TimingNode::update_slack_since_cell_move(){
    // if cell movable
        
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(get_pin_id());
    const timer::Timer &timer = timer::Timer::get_instance();     
    // Step1. update itself
    double new_placement_delay = 0.0;
    new_placement_delay = timer.get_displacement_delay_factor() * (abs(pin.get_x() - d_pin_input_pin_location.first) + abs(pin.get_y() - d_pin_input_pin_location.second));
    
    
    add_slack( get_input_pin_d_pin_placement_delay() - new_placement_delay);
    set_input_pin_d_pin_placement_delay(new_placement_delay);

}

void TimingNode::cell_move_update_affected_d_pin_slack(){
    ////std::cout<<"DEBUG cell: "<<get_pin_id()<<" cell_move_update_affected_d_pin_slack"<<std::endl;
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    timer::Timer &timer = timer::Timer::get_instance();      
    const circuit::Pin &pin = netlist.get_pin(get_pin_id());
    int x = pin.get_x();
    int y = pin.get_y();

    for(int affected_d_pin : affected_d_pins){        
        TimingNode &node = timer.get_mutable_timing_node(affected_d_pin);        

        double new_q_pin_output_pin_placement_delay;
        new_q_pin_output_pin_placement_delay = timer.get_displacement_delay_factor() * (abs(x- node.q_pin_output_pin_location.first) 
        + abs(y - node.q_pin_output_pin_location.second));

        node.add_slack(node.get_q_pin_output_pin_placement_delay() - new_q_pin_output_pin_placement_delay);
        node.set_q_pin_output_pin_placement_delay(new_q_pin_output_pin_placement_delay);
    }
}


void TimingNode::libcell_change_update_affected_d_pin_slack(double q_pin_delay){ 
    timer::Timer &timer = timer::Timer::get_instance();
    for(int affected_d_pin : affected_d_pins){
        TimingNode &node = timer.get_mutable_timing_node(affected_d_pin);
        node.add_slack(node.get_q_pin_delay() - q_pin_delay);
        node.set_q_pin_delay(q_pin_delay);        
    }
}


void Timer::init_timing(const std::unordered_map<int,double> &init_pin_slack_map){
    // set all d_pins init slack
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
    for(auto& [id,slack] : init_pin_slack_map){
        //MAPPING
        int mapping_pin_id = netlist.get_ff_pin_id(id);
        set_slack(mapping_pin_id,slack);
    }
}

//INIT Use ORIGINAL NETLIST
void Timer:: dfs_from_q_pin_to_each_d_pin(int q_pin_id){    
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
    const design::Design &design = design::Design::get_instance();

    // q_pin and q_pin delay
    const circuit::Pin &q_pin = netlist.get_pin(q_pin_id);
    if(q_pin.no_connection()){
        return;
    }
    int cell_id = q_pin.get_cell_id();    
    const circuit::Cell &cell = netlist.get_cell(cell_id);
    int libcell_id = cell.get_lib_cell_id();
    const design::LibCell &libcell = design.get_lib_cell(libcell_id);
    double q_pin_delay = libcell.get_delay();

    int net_id = q_pin.get_net_id();
    const circuit::Net &net = netlist.get_net(net_id);
    
    // q_pin slack does not matter 0.0
    // MAPPING
    int mapping_q_pin_id = netlist.get_ff_pin_id(q_pin_id);
    timing_nodes.insert(std::make_pair(mapping_q_pin_id, TimingNode(mapping_q_pin_id,0.0)));

    for(int pin_id : net.get_pins_id()){
        // skip q_pin
        if(pin_id == q_pin_id){
            continue;
        }
        // q_pin_output_pin
        const circuit::Pin &pin = netlist.get_pin(pin_id);
        if(pin.is_port()){           
            continue;
        }

        std::pair<int,int> q_pin_output_pin_location = std::make_pair(pin.get_x(),pin.get_y());
        double q_pin_output_pin_placement_delay = get_displacement_delay_factor() * (abs(q_pin.get_x() - pin.get_x()) + abs(q_pin.get_y() - pin.get_y()));                
        const std::vector<int> &affected_d_pins = dfs_until_d_pin_using_stack(q_pin_id,pin.get_id(),q_pin_output_pin_location,q_pin_output_pin_placement_delay,q_pin_delay);
        for(int d_pin_id : affected_d_pins){
            // MAPPING
            timing_nodes[mapping_q_pin_id].add_affected_d_pin(d_pin_id);
        }
    }
    
}

std::vector<int> Timer::dfs_until_d_pin_using_stack(int start_q_pin_id,int q_pin_output_pin_id,const std::pair<int,int> &q_pin_output_pin_location, const double q_pin_output_pin_placement_delay, const double q_pin_delay){
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();        
    std::vector<int> affected_d_pins;

    std::stack<std::pair<int,int>> sink_pins_stack;
    sink_pins_stack.push(std::make_pair(start_q_pin_id,q_pin_output_pin_id));

    while(!sink_pins_stack.empty()){
        int fanin_pid = sink_pins_stack.top().first;
        int sink_pin_id = sink_pins_stack.top().second;        
        sink_pins_stack.pop();
        const circuit::Pin &pin = netlist.get_pin(sink_pin_id);
        if(pin.is_port()){            
            continue;
        }
        else if(pin.is_d_pin()){
            // MAPPING
            int mapping_sink_pin_id = netlist.get_ff_pin_id(sink_pin_id);
            timing_nodes[mapping_sink_pin_id].set_q_pin_output_pin_location(q_pin_output_pin_location);
            timing_nodes[mapping_sink_pin_id].set_q_pin_output_pin_placement_delay(q_pin_output_pin_placement_delay);
            const circuit::Pin &d_pin_input_pin = netlist.get_pin(fanin_pid);            
            double input_pin_d_pin_placement_delay = get_displacement_delay_factor() * (abs(d_pin_input_pin.get_x() - pin.get_x()) + abs(d_pin_input_pin.get_y() - pin.get_y()));
            timing_nodes[mapping_sink_pin_id].set_input_pin_d_pin_placement_delay(input_pin_d_pin_placement_delay);        
            timing_nodes[mapping_sink_pin_id].set_d_pin_input_pin_location(std::make_pair(d_pin_input_pin.get_x(),d_pin_input_pin.get_y()));
            timing_nodes[mapping_sink_pin_id].set_q_pin_delay(q_pin_delay);
            // MAPPING            
            affected_d_pins.push_back(mapping_sink_pin_id);            
        }else{
            // get outputs pin from input pin
            int cell_id = pin.get_cell_id();
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            for(int output_pin_id : cell.get_output_pins_id()){
                const circuit::Pin &output_pin = netlist.get_pin(output_pin_id);
                if(output_pin.no_connection()){
                    continue;
                }
                int output_net_id = output_pin.get_net_id();
                const circuit::Net &output_net = netlist.get_net(output_net_id);                                
                for(int next_input_pin_id : output_net.get_pins_id()){
                    if(next_input_pin_id == output_pin_id){
                        continue;
                    }
                    sink_pins_stack.push(std::make_pair(output_pin_id,next_input_pin_id));
                }
            }
        }        
    }
    return affected_d_pins;
}

void Timer::create_timing_graph_by_read_data(){    
    config::ConfigManager &config_manager = config::ConfigManager::get_instance();
    int read_case = std::get<int>(config_manager.get_config_value("timer_case"));
    std::string read_case_str;
    std::string pre_fix = "timer_init/";
    if(read_case == 0){
        read_case_str = "timer_input.txt";
    }else if(read_case == 1){
        read_case_str = "timer_testcase1_0812.txt";
    }else if(read_case == 2){
        read_case_str = "timer_testcase2_0812.txt";
    }else if(read_case == 3){
        read_case_str = "timer_tiny.txt";
    }else if(read_case == 4){
        read_case_str = "timer_testcase3.txt";
    }
    const std::string &path = pre_fix + read_case_str;
    std::ifstream file(path);
    std::string line;
    while(std::getline(file,line)){
        std::istringstream iss(line);
        int id;
        double q_pin_delay;
        double q_pin_output_pin_placement_delay;
        double input_pin_d_pin_placement_delay;
        double slack;
        int q_pin_output_pin_x;
        int q_pin_output_pin_y;
        int d_pin_input_pin_x;
        int d_pin_input_pin_y;
        int size;
        iss >> id >> q_pin_delay >> q_pin_output_pin_placement_delay >> input_pin_d_pin_placement_delay >> slack >> q_pin_output_pin_x >> q_pin_output_pin_y >> d_pin_input_pin_x >> d_pin_input_pin_y >> size;
        std::pair<int,int> q_pin_output_pin_location = std::make_pair(q_pin_output_pin_x,q_pin_output_pin_y);
        std::pair<int,int> d_pin_input_pin_location = std::make_pair(d_pin_input_pin_x,d_pin_input_pin_y);
        TimingNode node(id,slack);
        node.set_q_pin_delay(q_pin_delay);
        node.set_q_pin_output_pin_placement_delay(q_pin_output_pin_placement_delay);
        node.set_input_pin_d_pin_placement_delay(input_pin_d_pin_placement_delay);
        node.set_q_pin_output_pin_location(q_pin_output_pin_location);
        node.set_d_pin_input_pin_location(d_pin_input_pin_location);        
        for(int i=0;i<size;i++){
            int affected_d_pin;
            iss >> affected_d_pin;
            node.add_affected_d_pin(affected_d_pin);
        }
        timing_nodes.insert(std::make_pair(id,node));
    }
}

void Timer::create_timing_graph(){
    const config::ConfigManager &config_manager = config::ConfigManager::get_instance();
    bool read_data = std::get<bool>(config_manager.get_config_value("fast_timer")); 
    if(read_data){
        create_timing_graph_by_read_data();
        return;
    }
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(const auto &cell : cells){
        if(!cell.is_sequential()){
            continue;
        }       
        for(int q_pin_id : cell.get_output_pins_id()){     
            dfs_from_q_pin_to_each_d_pin(q_pin_id);
        }
    }
    write_init_timing_data();
    //std::cout<<"DEBUG create_timing_graph Finish "<<std::endl;
}

void Timer::update_slack_since_cell_move(int cell_id){

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Cell &cell = netlist.get_cell(cell_id);    
    for(int d_pin_id : cell.get_input_pins_id()){        
        if(timing_nodes.find(d_pin_id) == timing_nodes.end()){
            continue;
        }
        timing_nodes.at(d_pin_id).update_slack_since_cell_move();
    }

    for(int q_pin_id : cell.get_output_pins_id()){        
        if(timing_nodes.find(q_pin_id) == timing_nodes.end()){
            continue;
        }
        timing_nodes.at(q_pin_id).cell_move_update_affected_d_pin_slack();        
    }
}

void Timer::update_slack_since_libcell_change(int cell_id){
    //std::cout<<"DEBUG cell: "<<cell_id<<" update_slack_since_libcell_change"<<std::endl;    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const circuit::Cell &cell = netlist.get_cell(cell_id);    
    const design::Design &design = design::Design::get_instance();
    int libcell_id = cell.get_lib_cell_id();
    const design::LibCell &libcell = design.get_lib_cell(libcell_id);
    double q_pin_delay = libcell.get_delay();    
    for(int q_pin_id : cell.get_output_pins_id()){
        timing_nodes[q_pin_id].libcell_change_update_affected_d_pin_slack(q_pin_delay);
    }
}

void Timer::update_timing(int cell_id){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
    update_slack_since_cell_move(cell_id);
    update_slack_since_libcell_change(cell_id);
    cell.calculate_slack();
}

void Timer::update_timing(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cell_id : sequential_cells_id){
        update_timing(cell_id);
    }    
}

void Timer::switch_to_other_solution(const std::unordered_map<int,TimingNode> &timing_nodes){
    this->timing_nodes = timing_nodes;
    // update cells slack
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cell_id : sequential_cells_id){
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        cell.calculate_slack();
    }
}


} // namespace timer