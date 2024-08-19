#include "timer.h"
#include <cmath>
#include <queue>
#include <stack>
#include <../circuit/netlist.h>
#include <../design/design.h>
#include <../design/libcell.h>
#include <../circuit/cell.h>
#include <../circuit/pin.h>
#include <../circuit/net.h>
namespace timer {

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
    for(auto& [id,slack] : init_pin_slack_map){
        set_slack(id,slack);
    }
}

void Timer:: dfs_from_q_pin_to_each_d_pin(int q_pin_id){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const design::Design &design = design::Design::get_instance();

    // q_pin and q_pin delay
    const circuit::Pin &q_pin = netlist.get_pin(q_pin_id);
    int cell_id = q_pin.get_cell_id();    
    const circuit::Cell &cell = netlist.get_cell(cell_id);
    int libcell_id = cell.get_lib_cell_id();
    const design::LibCell &libcell = design.get_lib_cell(libcell_id);
    double q_pin_delay = libcell.get_delay();

    // debug
    const std::string &cell_name = netlist.get_cell_name(cell_id);

    // Traverse netlist from q_pin_output_pin
    int net_id = q_pin.get_net_id();
    const circuit::Net &net = netlist.get_net(net_id);

    const std::string &net_name = netlist.get_net_name(net_id);
    ////std::cout<<"DEBUG net: "<<net_name<<" dfs find dpin path"<<std::endl;

    timing_nodes.insert(std::make_pair(q_pin_id, TimingNode(q_pin_id,0.0)));

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
        //int d_pin_id = dfs_until_d_pin(q_pin_id,pin.get_id(),q_pin_output_pin_location,q_pin_output_pin_placement_delay,q_pin_delay);        
        const std::vector<int> &affected_d_pins = dfs_until_d_pin_using_stack(q_pin_id,pin.get_id(),q_pin_output_pin_location,q_pin_output_pin_placement_delay,q_pin_delay);
        for(int d_pin_id : affected_d_pins){
            timing_nodes[q_pin_id].add_affected_d_pin(d_pin_id);
        }
    }
    
}

int Timer::dfs_until_d_pin(int fanin_pid,int pid,const std::pair<int,int> &q_pin_output_pin_location, const double q_pin_output_pin_placement_delay, const double q_pin_delay){        
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(pid);
    if(pin.is_port()){
        return -1;
    }
    
    if(pin.is_d_pin()){
        timing_nodes[pid].set_q_pin_output_pin_location(q_pin_output_pin_location);
        timing_nodes[pid].set_q_pin_output_pin_placement_delay(q_pin_output_pin_placement_delay);
        const circuit::Pin &d_pin_input_pin = netlist.get_pin(fanin_pid);
        double input_pin_d_pin_placement_delay = get_displacement_delay_factor() * (abs(d_pin_input_pin.get_x() - pin.get_x()) + abs(d_pin_input_pin.get_y() - pin.get_y()));
        timing_nodes[pid].set_input_pin_d_pin_placement_delay(input_pin_d_pin_placement_delay);        
        timing_nodes[pid].set_d_pin_input_pin_location(std::make_pair(d_pin_input_pin.get_x(),d_pin_input_pin.get_y()));
        timing_nodes[pid].set_q_pin_delay(q_pin_delay);
        return pid;
    }

    // get outputs pin from input pin
    int cell_id = pin.get_cell_id();
    const circuit::Cell &cell = netlist.get_cell(cell_id);
    for(int output_pin_id : cell.get_output_pins_id()){
        const circuit::Pin &output_pin = netlist.get_pin(output_pin_id);
        int output_net_id = output_pin.get_net_id();
        const circuit::Net &output_net = netlist.get_net(output_net_id);        
        for(int next_input_pin_id : output_net.get_pins_id()){
            if(next_input_pin_id == output_pin_id){
                continue;
            }
            dfs_until_d_pin(output_pin_id,next_input_pin_id,q_pin_output_pin_location,q_pin_output_pin_placement_delay,q_pin_delay);
        }
    }
    return -1;
}

std::vector<int> Timer::dfs_until_d_pin_using_stack(int start_q_pin_id,int q_pin_output_pin_id,const std::pair<int,int> &q_pin_output_pin_location, const double q_pin_output_pin_placement_delay, const double q_pin_delay){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();        
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
            timing_nodes[sink_pin_id].set_q_pin_output_pin_location(q_pin_output_pin_location);
            timing_nodes[sink_pin_id].set_q_pin_output_pin_placement_delay(q_pin_output_pin_placement_delay);
            const circuit::Pin &d_pin_input_pin = netlist.get_pin(fanin_pid);            
            double input_pin_d_pin_placement_delay = get_displacement_delay_factor() * (abs(d_pin_input_pin.get_x() - pin.get_x()) + abs(d_pin_input_pin.get_y() - pin.get_y()));
            timing_nodes[sink_pin_id].set_input_pin_d_pin_placement_delay(input_pin_d_pin_placement_delay);        
            timing_nodes[sink_pin_id].set_d_pin_input_pin_location(std::make_pair(d_pin_input_pin.get_x(),d_pin_input_pin.get_y()));
            timing_nodes[sink_pin_id].set_q_pin_delay(q_pin_delay);
            affected_d_pins.push_back(sink_pin_id);            
        }else{
            // get outputs pin from input pin
            int cell_id = pin.get_cell_id();
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            for(int output_pin_id : cell.get_output_pins_id()){
                const circuit::Pin &output_pin = netlist.get_pin(output_pin_id);
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


void Timer::create_timing_graph(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequentail_cells_id = netlist.get_sequential_cells_id();
    for(int cell_id : sequentail_cells_id){
        const circuit::Cell &cell = cells.at(cell_id);
        for(int q_pin_id : cell.get_output_pins_id()){     
            dfs_from_q_pin_to_each_d_pin(q_pin_id);
        }
    }
    //std::cout<<"DEBUG create_timing_graph Finish "<<std::endl;
}

void Timer::update_slack_since_cell_move(int cell_id){
    //std::cout<<"TIMER:: cell: "<<cell_id<<" update_slack_since_cell_move"<<std::endl;

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Cell &cell = netlist.get_cell(cell_id);    
    //std::cout<<"TIMER:: cell: "<<cell_id<<" update_slack_since_cell_move itself"<<std::endl;
    for(int d_pin_id : cell.get_input_pins_id()){        
        if(timing_nodes.find(d_pin_id) == timing_nodes.end()){
            //std::cout<<"DEBUG cell: "<<cell_id<<" d_pin_id: "<<d_pin_id<<" not found"<<std::endl;
            continue;
        }
        timing_nodes.at(d_pin_id).update_slack_since_cell_move();
    }

    //std::cout<<"TIMER:: cell: "<<cell_id<<" update_slack_since_cell_move next affected ffs"<<std::endl;
    for(int q_pin_id : cell.get_output_pins_id()){        
        if(timing_nodes.find(q_pin_id) == timing_nodes.end()){
            //std::cout<<"DEBUG cell: "<<cell_id<<" q_pin_id: "<<q_pin_id<<" not found"<<std::endl;
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
    //std::cout<<"TIMER:: update_timing cell_id:"<<cell_id<<std::endl;
    circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
    if(cell.is_clustered()){
        const std::string &cell_name = netlist.get_cell_name(cell_id);    
        //std::cout<<"DEBUG cell: "<<cell_name<<" is clustered"<<std::endl;
        return;
    }    
    ////std::cout<<"DEBUG cell: "<<cell_name<<" id:"<<cell_id<<" update timing"<<std::endl;

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