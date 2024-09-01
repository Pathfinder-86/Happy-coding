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


void OutputDelayNode::propagate_delay(){
    Timer &timer = Timer::get_instance();
    for(int input_delay_node_id : input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(input_delay_node_id).update( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
}

void OutputDelayNode::init_propagate_delay(){
    Timer &timer = Timer::get_instance();
    for(int input_delay_node_id : input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(input_delay_node_id).update_init_delay( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).init( get_delay() );
    }
}

void InputDelayNode::update_to_pins_delay(){
    Timer &timer = Timer::get_instance();
    for(int to_pin_id : to_pins_id){
        OutputDelayNode &output_delay_node = timer.get_mutable_output_delay_node(to_pin_id);
        output_delay_node.update_delay(get_delay());
    }
}

void InputDelayNode::update(double init_delay){
    update_init_delay(init_delay);
    Timer &timer = Timer::get_instance();
    double displacement_delay = timer.get_displacement_delay_factor();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const circuit::Pin &pin = original_netlist.get_pin(id);
    double propagation_delay = 0.0;
    if(get_is_from_pin_q_pin()){
        const circuit::Netlist &netlist = circuit::Netlist::get_instance();
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);                   
        propagation_delay = displacement_delay * (abs(q_pin.get_x() - pin.get_x()) + abs(q_pin.get_y() - pin.get_y()));                
    }else{
        const circuit::Pin &output_pin = original_netlist.get_pin(from_pin_id);
        propagation_delay = displacement_delay * (abs(output_pin.get_x() - pin.get_x()) + abs(output_pin.get_y() - pin.get_y()));
    }
    update_propagation_delay(propagation_delay);
    update_to_pins_delay();    
}

void QpinNode::init_propagate_delay(){
    timer::Timer &timer = Timer::get_instance();
    for(int input_delay_node_id : input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(input_delay_node_id).update( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).init( get_delay() );
    }
}

void QpinNode::propagate_delay(){
    timer::Timer &timer = Timer::get_instance();
    for(int input_delay_node_id : input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(input_delay_node_id).update( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
}
void QpinNode::update(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(id);
    const circuit::Cell &cell = netlist.get_cell(pin.get_cell_id());
    double q_pin_delay = cell.get_delay();
    set_delay(q_pin_delay);
    propagate_delay();
}

void DpinNode::init(double delay){    
    set_init_delay(delay);
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &d_pin = netlist.get_pin(id);
    Timer &timer = Timer::get_instance();
    double displacement_delay = timer.get_displacement_delay_factor();
    double propagation_delay = 0.0;
    if(get_is_from_pin_q_pin()){                                
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);            
        propagation_delay = displacement_delay * (abs(q_pin.get_x() - d_pin.get_x()) + abs(q_pin.get_y() - d_pin.get_y()));                
    }else{                    
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        const circuit::Pin &output_pin = original_netlist.get_pin(from_pin_id);
        propagation_delay = displacement_delay * (abs(output_pin.get_x() - d_pin.get_x()) + abs(output_pin.get_y() - d_pin.get_y()));        
    }
    set_propagation_delay(propagation_delay);
}

void DpinNode::update(double delay){    
    update_init_delay(delay);    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &d_pin = netlist.get_pin(id);
    Timer &timer = Timer::get_instance();
    double displacement_delay = timer.get_displacement_delay_factor();    
    if(get_is_from_pin_q_pin()){                                
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);            
        double new_propagation_delay = displacement_delay * (abs(q_pin.get_x() - d_pin.get_x()) + abs(q_pin.get_y() - d_pin.get_y()));              
        update_propagation_delay(new_propagation_delay);
    }

    timer.add_affected_d_pin_id(id);
}

void DpinNode::update_location(){    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &d_pin = netlist.get_pin(id);
    Timer &timer = Timer::get_instance();
    double displacement_delay = timer.get_displacement_delay_factor();
    double new_propagation_delay = 0.0;
    if(get_is_from_pin_q_pin()){                                
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);            
        new_propagation_delay = displacement_delay * (abs(q_pin.get_x() - d_pin.get_x()) + abs(q_pin.get_y() - d_pin.get_y()));                
    }else{                    
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        const circuit::Pin &output_pin = original_netlist.get_pin(from_pin_id);
        new_propagation_delay = displacement_delay * (abs(output_pin.get_x() - d_pin.get_x()) + abs(output_pin.get_y() - d_pin.get_y()));        
    }
    update_propagation_delay(new_propagation_delay);

    timer.add_affected_d_pin_id(id);
}


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
    /*
    std::ofstream file(path);
    for(auto& [id,timing_node] : dpin_nodes){
    }*/
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
    const std::vector<circuit::Net> &nets = netlist.get_nets();
    // add d_pins, q_pins, input_delay_nodes, and output_delay_nodes
    for(const auto &net : nets){
        for(int pin_id : net.get_pins_id()){
            const circuit::Pin &pin = netlist.get_pin(pin_id);        
            if(pin.get_is_port() || pin.get_is_clk()){
                continue;
            }
            if(pin.is_ff_pin()){
                int mapping_pin_id = netlist.get_ff_pin_id(pin_id);
                if(pin.get_is_input()){                               
                    dpin_nodes.insert(std::make_pair(mapping_pin_id,DpinNode(mapping_pin_id)));
                }else{
                    qpin_nodes.insert(std::make_pair(mapping_pin_id,QpinNode(mapping_pin_id)));
                }
            }
            else{
                if(pin.get_is_input()){
                    input_delay_nodes.insert(std::make_pair(pin_id,InputDelayNode(pin_id)));                    
                }else{
                    output_delay_nodes.insert(std::make_pair(pin_id,OutputDelayNode(pin_id)));
                }
            }
        }    
    }    
    add_intput_delay_node_to_pins_id();    
    traverse_each_net_to_add_connection();
    init_propagate();
    //std::cout<<"DEBUG create_timing_graph Finish "<<std::endl;
}

void Timer::add_intput_delay_node_to_pins_id(){
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(const auto &cell : cells){
        if(cell.is_sequential()){
            continue;
        }                
        const std::vector<int> &input_pins_id = cell.get_input_pins_id();
        const std::vector<int> &output_pins_id = cell.get_output_pins_id();
        for(int input_pin_id : input_pins_id){
            for(int output_pin_id : output_pins_id){
                input_delay_nodes[input_pin_id].add_to_pins_id(output_pin_id);
            }
        }
    }
}

void Timer::traverse_each_net_to_add_connection(){
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
    const std::vector<circuit::Net> &nets = netlist.get_nets();
    for(const auto &net : nets){
        if(net.get_is_clock_net()){
            continue;
        }
        int driver_pin_id = net.get_driver_pin_id();
        const circuit::Pin &driver_pin = netlist.get_pin(driver_pin_id);
        bool is_q_pin = driver_pin.is_ff_pin();
        if(is_q_pin){
            int mapping_q_pin_id = netlist.get_ff_pin_id(driver_pin_id);
            for(int sink_pin_id : net.get_pins_id()){
                if(sink_pin_id == driver_pin_id){
                    continue;
                }
                const circuit::Pin &sink_pin = netlist.get_pin(sink_pin_id);
                if(sink_pin.get_is_port() || sink_pin.get_is_clk()){
                    continue;
                }
                if(sink_pin.is_d_pin()){
                    // MAPPING
                    int mapping_d_pin_id = netlist.get_ff_pin_id(sink_pin_id);
                    qpin_nodes[mapping_q_pin_id].add_d_pin_node(mapping_d_pin_id);
                    dpin_nodes[mapping_d_pin_id].set_from_pin_id(mapping_q_pin_id);
                    dpin_nodes[mapping_d_pin_id].set_is_from_pin_q_pin(true);
                }else{
                    qpin_nodes[mapping_q_pin_id].add_input_delay_node(sink_pin_id);
                    input_delay_nodes[sink_pin_id].set_is_from_pin_q_pin(true);
                    input_delay_nodes[sink_pin_id].set_from_pin_id(mapping_q_pin_id);
                }
            }
        }else{            
            for(int sink_pin_id : net.get_pins_id()){
                if(sink_pin_id == driver_pin_id){
                    continue;
                }
                const circuit::Pin &sink_pin = netlist.get_pin(sink_pin_id);
                if(sink_pin.get_is_port() || sink_pin.get_is_clk()){
                    continue;
                }
                if(sink_pin.is_d_pin()){
                    // MAPPING
                    int mapping_d_pin_id = netlist.get_ff_pin_id(sink_pin_id);
                    output_delay_nodes[driver_pin_id].add_d_pin_node(mapping_d_pin_id);
                    dpin_nodes[mapping_d_pin_id].set_from_pin_id(driver_pin_id);
                    dpin_nodes[mapping_d_pin_id].set_is_from_pin_q_pin(false);
                }else{
                    output_delay_nodes[driver_pin_id].add_input_delay_node(sink_pin_id);
                    input_delay_nodes[sink_pin_id].set_is_from_pin_q_pin(false);
                    input_delay_nodes[sink_pin_id].set_from_pin_id(driver_pin_id);
                }
            }
        }                
    }
}

const std::unordered_set<int> & Timer::get_affected_cells_id_by_affected_d_pins_id(){
    std::unordered_set<int> affected_cells_id_set;    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int d_pin_id : affected_d_pins_id_set){
        const circuit::Pin &d_pin = netlist.get_pin(d_pin_id);
        int cell_id = d_pin.get_cell_id();
        affected_cells_id_set.insert(cell_id);
    }

    for(int cell_id : affected_cells_id_set){
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        cell.calculate_slack();
    }
    reset_affetced_d_pins_id_set();
    return affected_cells_id_set;        
}

void Timer::update_timing(int cell_id){    
    reset_affetced_d_pins_id_set();
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const circuit::Cell &cell = netlist.get_cell(cell_id);
    // location update
    for(int d_pin_id : cell.get_input_pins_id()){
        dpin_nodes[d_pin_id].update_location();        
    }    

    // propagate delay
    for(int q_pin_id : cell.get_output_pins_id()){
        qpin_nodes[q_pin_id].update();
    }    
}

void Timer::update_timing(){    
    for(auto &it : qpin_nodes){
        it.second.update();
    }
}

void Timer::init_propagate(){
    for(auto &it : qpin_nodes){
        it.second.init_propagate_delay();
    }
}


void Timer::switch_to_other_solution(const std::unordered_map<int,DpinNode> &dpin_nodes,const std::unordered_map<int,QpinNode> &qpin_nodes,const std::unordered_map<int,InputDelayNode> &input_delay_nodes,const std::unordered_map<int,OutputDelayNode> &output_delay_nodes){
    this -> dpin_nodes = dpin_nodes;
    this -> qpin_nodes = qpin_nodes;
    this -> input_delay_nodes = input_delay_nodes;
    this -> output_delay_nodes = output_delay_nodes;
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(auto &cell : netlist.get_mutable_cells()){
        cell.calculate_slack();
    }
}

void Timer::update_timing_by_cells_id(const std::vector<int> &cells_id){
    for(int cell_id : cells_id){
        update_timing(cell_id);
    }
}

double Timer::calculate_timing_cost_after_cluster(const std::vector<int> &cells_id, std::vector<int> &affected_cells_id){    
    double cost_change = 0.0;
    const design::Design &design = design::Design::get_instance();
    double timing_factor = design.get_timing_factor();
    update_timing_by_cells_id(cells_id);
    const std::unordered_set<int> &affected_cells_id_set = get_affected_cells_id_by_affected_d_pins_id();
    double original_slack_cost = 0.0;
    for(int cell_id : affected_cells_id_set){
        const circuit::Cell &cell = circuit::Netlist::get_instance().get_cell(cell_id);
        if(cell.get_slack() < 0){
            original_slack_cost += cell.get_slack() * timing_factor;
        }

    }

    double new_slack_cost = 0.0;
    update_cells_slack(affected_cells_id_set);
    for(int cell_id : affected_cells_id_set){
        const circuit::Cell &cell = circuit::Netlist::get_instance().get_cell(cell_id);
        if(cell.get_slack() < 0){
            new_slack_cost += cell.get_slack() * timing_factor;
        }
    }

    cost_change = new_slack_cost - original_slack_cost;    
    affected_cells_id.insert(affected_cells_id.end(),affected_cells_id_set.begin(),affected_cells_id_set.end());
    return cost_change;
}

void Timer::update_cells_slack(const std::unordered_set<int> &affected_cells_id){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int cell_id : affected_cells_id){
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        cell.calculate_slack();
    }
}

} // namespace timer