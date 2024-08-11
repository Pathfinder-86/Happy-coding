#include "timer.h"
#include <cmath>
#include <queue>
#include <../circuit/netlist.h>
namespace timer {
void Timer::init_timing(const std::unordered_map<int,double> &init_pin_slack_map){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::queue<int> Queue;
    std::unordered_set<int> visited;


    // D pin use edges propagate to D pin first
    for(auto& [id,slack] : init_pin_slack_map){
        //std::cout << "id: " << id << " slack: " << slack << std::endl;
        timing_nodes[id].set_slack(slack);
        visited.insert(id);
        Queue.push(id);
    }
    // level order traversal
    std::cout<<"propagate slack"<<std::endl;

    while(!Queue.empty()){
        int size = Queue.size();
        for(int i=0;i<size;i++){
            int node_id = Queue.front();
            //std::cout<<"node_id: "<<node_id<<std::endl;
            Queue.pop();
            if(timing_graph.find(node_id) == timing_graph.end()){
                const std::string &pin_name = netlist.get_pin_name(node_id);
                //std::cout<<"no node_id:"<<node_id<<" name:"<<pin_name<<" in timing_graph"<<std::endl;                
                continue;
            }

            for(int sink_id : timing_graph.at(node_id)){
                //std::cout<<"get edge delay"<<std::endl;
                double delay = timing_edges.at(node_id).at(sink_id).get_delay();
                //std::cout<<"get driver slack"<<std::endl;
                double new_slack = timing_nodes.at(node_id).get_slack() + delay;                                
                //std::cout<<"get sink slack:"<<new_slack<<std::endl;
                double origin_slack = timing_nodes.at(sink_id).get_slack();
                double max_slack = std::max(origin_slack,new_slack);
                if(max_slack != origin_slack){                    
                    timing_nodes.at(sink_id).set_slack(max_slack);                    
                }

                if(visited.find(sink_id) == visited.end()){
                    visited.insert(sink_id);
                    Queue.push(sink_id);
                }
            }
        }
    }

    set_dirty(false);
    return;
}

void Timer::add_net_into_timing_graph(const circuit::Net& net){
    const design::Design& design = design::Design::get_instance();
    const circuit::Netlist& netlist = circuit::Netlist::get_instance();
    int driver_pin_id = net.get_driver_pin_id();
    int net_id = net.get_id();
    const std::string &net_name = netlist.get_net_name(net_id);
    if(driver_pin_id == -1){
        // no driver pin :O
        //std::cout<<"no driver pin in net:"<<net_name<<std::endl;
        return;
    }   

    double displacement_delay = design.get_displacement_delay();    
    const circuit::Pin& driver_pin = netlist.get_pin(driver_pin_id);
    const std::string &driver_pin_name = netlist.get_pin_name(driver_pin_id);
    //std::cout<<"driver_pin_id: "<<driver_pin_id<<" driver_pin_name: "<<driver_pin_name<<std::endl;

    for(int pin_id : net.get_pins_id()){
        TimingNode node(pin_id);        
        timing_nodes[pin_id] = node;

        if(pin_id == driver_pin_id){            
            continue;
        }
        // add nodes connected to driver
        timing_graph[driver_pin_id].insert(pin_id);        
        // add edge from driver to other pins        
        const circuit::Pin& sink_pin = netlist.get_pin(pin_id);
        int w_diff = driver_pin.get_x() - sink_pin.get_x();
        int h_diff = driver_pin.get_y() - sink_pin.get_y();
        double delay = displacement_delay * (abs(w_diff) + abs(h_diff));
        TimingEdge edge(delay);
        timing_edges[driver_pin_id][pin_id] = edge;

        // debug
        //std::cout<<driver_pin_id<<" -> "<<pin_id<<" delay:"<<delay<<std::endl;
    }

    // no propagation all slack are wrong here
    std::cout<<"add net into timing graph finish"<<std::endl;
    return;
}

void Timer::add_cell_delay_into_timing_graph(const circuit::Cell &cell){
    std::vector<int> input_pins_id = cell.get_input_pins_id();
    std::vector<int> output_pins_id = cell.get_output_pins_id();    
    for(int input_pin_id : input_pins_id){
        TimingNode node(input_pin_id);        
        timing_nodes[input_pin_id] = node;
    }
    for(int output_pin_id : output_pins_id){
        TimingNode node(output_pin_id);        
        timing_nodes[output_pin_id] = node;
    }    
    for(int input_pin_id : input_pins_id){    
        for(int output_pin_id : output_pins_id){                        
            timing_graph[input_pin_id].insert(output_pin_id);
            double delay = cell.get_delay();
            TimingEdge edge(delay);
            timing_edges[input_pin_id][output_pin_id] = edge;
        }
    }    

}



}