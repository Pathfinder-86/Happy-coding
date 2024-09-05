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
#include <algorithm>
namespace timer {


void OutputDelayNode::propagate_delay(){
    Timer &timer = Timer::get_instance();        
    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){                           
        timer.get_mutable_input_delay_node(fanout_input_delay_node_id).update( get_delay() );
    }

    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
}

void OutputDelayNode::update(double delay, int input_pin_id){
    timer::Timer &timer = Timer::get_instance();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::string &output_pin_name = original_netlist.get_pin_name(id);
    if(delay > get_delay()){
        set_delay(delay);
        set_max_delay_input_pin_id(input_pin_id);
        propagate_delay();
        return;
    }else if(input_pin_id == get_max_delay_input_pin_id() && delay < get_delay()){        
        double new_delay = 0.0;
        int new_input_pin_id = -1;
        for(int input_delay_node_id : input_delay_nodes_id){           
            double input_node_delay = timer.get_input_delay_node(input_delay_node_id).get_delay();
            if(input_node_delay >= new_delay){
                new_delay = input_node_delay;
                new_input_pin_id = input_delay_node_id;
            }
        }
        set_delay(new_delay);
        set_max_delay_input_pin_id(new_input_pin_id);
        propagate_delay();
    }    
}

void OutputDelayNode::init(double delay, int input_pin_id){
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::string &output_pin_name = original_netlist.get_pin_name(id);
    const std::string &input_pin_name = original_netlist.get_pin_name(input_pin_id);
    //std::cout<<"INIT: "<<output_pin_name<<" from input_pin"<<input_pin_name<<" delay: "<<delay<<std::endl;

    if(delay > get_delay()){        
        set_delay(delay);
        set_max_delay_input_pin_id(input_pin_id);        
        init_propagate_delay();        
    }else{
        //std::cout<<"Less than max delay:"<<get_delay()<<std::endl;
    }
}

void OutputDelayNode::init_propagate_delay(){
    Timer &timer = Timer::get_instance();
    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(fanout_input_delay_node_id).init( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).init( get_delay() );
    }
}

void InputDelayNode::update_to_pins_delay(){
    Timer &timer = Timer::get_instance();
    for(int to_pin_id : to_pins_id){
        OutputDelayNode &output_delay_node = timer.get_mutable_output_delay_node(to_pin_id);
        output_delay_node.update(get_delay(),get_id());
    }
}

void InputDelayNode::init_to_pins_delay(){
    Timer &timer = Timer::get_instance();
    for(int to_pin_id : to_pins_id){
        OutputDelayNode &output_delay_node = timer.get_mutable_output_delay_node(to_pin_id);
        output_delay_node.init(get_delay(),get_id());
    }
}

void InputDelayNode::update(double init_delay){
    update_init_delay(init_delay);    
    if(get_is_from_pin_q_pin()){
        Timer &timer = Timer::get_instance();
        double displacement_delay = timer.get_displacement_delay_factor();
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        const circuit::Pin &pin = original_netlist.get_pin(id);        
        const circuit::Netlist &netlist = circuit::Netlist::get_instance();
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);                   
        double propagation_delay = displacement_delay * (abs(q_pin.get_x() - pin.get_x()) + abs(q_pin.get_y() - pin.get_y()));                
        update_propagation_delay(propagation_delay);
    }    
    update_to_pins_delay();    
}

void InputDelayNode::init(double init_delay){
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
    init_to_pins_delay();
}

void QpinNode::init_propagate_delay(){
    timer::Timer &timer = Timer::get_instance();
    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){           
        timer.get_mutable_input_delay_node(fanout_input_delay_node_id).init( get_delay() );
    }
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).init( get_delay() );
    }
}

void QpinNode::propagate_delay(){
    timer::Timer &timer = Timer::get_instance();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &q_pin = netlist.get_pin(id);

    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){      
        timer.get_mutable_input_delay_node(fanout_input_delay_node_id). update( get_delay());
    }   
    for(int d_pin_node_id : d_pin_nodes_id){            
        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
}

void QpinNode::update(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(id);
    const circuit::Cell &cell = netlist.get_cell(pin.get_cell_id());    
    set_delay(cell.get_delay());
    propagate_delay();
}

void QpinNode::init(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(id);
    const circuit::Cell &cell = netlist.get_cell(pin.get_cell_id());
    double q_pin_delay = cell.get_delay();
    set_delay(q_pin_delay);
    init_propagate_delay();
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
    Timer &timer = Timer::get_instance();   
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    int origin_d_pin_id = original_netlist.get_original_pin_id(id);
    const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);
    //std::cout<<"d_pin: "<<d_pin_name<<" slack change since previous q_pin"<<std::endl;

    if(get_is_from_pin_q_pin()){
        const circuit::Netlist &netlist = circuit::Netlist::get_instance();
        const circuit::Pin &d_pin = netlist.get_pin(id);             
        double displacement_delay = timer.get_displacement_delay_factor();                                   
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);            
        double new_propagation_delay = displacement_delay * (abs(q_pin.get_x() - d_pin.get_x()) + abs(q_pin.get_y() - d_pin.get_y()));                              


        int origin_q_pin_id = original_netlist.get_original_pin_id(from_pin_id);
        const std::string &q_pin_name = original_netlist.get_pin_name(origin_q_pin_id);
        //std::cout<<"q_pin: "<<q_pin_name<<" directly connect to d_pin"<<std::endl;

        update_propagation_delay(new_propagation_delay);        
    }
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &d_pin = netlist.get_pin(id);
    int cell_id = d_pin.get_cell_id();    
    timer.add_affected_d_pin_id(id);
}

void DpinNode::update_propagation_delay(double delay){
    double old_delay = propagation_delay;
    double slack_change = old_delay - delay;
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    int origin_d_pin_id = original_netlist.get_original_pin_id(id);
    const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);
    //std::cout<<"d_pin: "<<d_pin_name<<" propagation_delay slack_change: "<<slack_change<<std::endl;

    add_slack(slack_change);
    this -> propagation_delay = delay;
}
void DpinNode::update_init_delay(double delay){
    double old_delay = init_delay;
    double slack_change = old_delay - delay;
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    int origin_d_pin_id = original_netlist.get_original_pin_id(id);
    const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);
    //std::cout<<"d_pin: "<<d_pin_name<<" init_delay slack_change: "<<slack_change<<std::endl;

    add_slack(slack_change);
    this -> init_delay = delay;            
}

void DpinNode::update_location(){    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &d_pin = netlist.get_pin(id);
    Timer &timer = Timer::get_instance();
    double displacement_delay = timer.get_displacement_delay_factor();    
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    double new_propagation_delay = 0.0;
    
    int origin_d_pin_id = original_netlist.get_original_pin_id(id);
    const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);
    //std::cout<<"d_pin: "<<d_pin_name<<" location change"<<std::endl;

    if(get_is_from_pin_q_pin()){                                
        const circuit::Pin &q_pin = netlist.get_pin(from_pin_id);            
        new_propagation_delay = displacement_delay * (abs(q_pin.get_x() - d_pin.get_x()) + abs(q_pin.get_y() - d_pin.get_y()));        
    }else{                            
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
    const std::vector<circuit::Pin> &pins = netlist.get_pins();
    for(const auto &pin : pins){
        if(pin.get_is_clk()){
            continue;
        }
        if(pin.is_d_pin()){
            int mapping_pin_id = netlist.get_ff_pin_id(pin.get_id());
            dpin_nodes[mapping_pin_id] = DpinNode(mapping_pin_id);
        }else if(pin.is_q_pin()){
            int mapping_pin_id = netlist.get_ff_pin_id(pin.get_id());
            qpin_nodes[mapping_pin_id] = QpinNode(mapping_pin_id);
        }else if(pin.get_is_output()){
            output_delay_nodes[pin.get_id()] = OutputDelayNode(pin.get_id());
        }else if(pin.get_is_input()){
            input_delay_nodes[pin.get_id()] = InputDelayNode(pin.get_id());            
        }
    }
    add_intput_delay_node_to_pins_id();    
    traverse_each_net_to_add_connection();
    init_propagate();
    ////std::cout<<"DEBUG create_timing_graph Finish "<<std::endl;
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
        for(int output_pin_id : output_pins_id){
            for(int input_pin_id : input_pins_id){
                output_delay_nodes[output_pin_id].add_input_delay_node_id(input_pin_id);
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
                    qpin_nodes[mapping_q_pin_id].add_fanout_input_delay_node_id(sink_pin_id);
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
                    output_delay_nodes[driver_pin_id].add_fanout_input_delay_node_id(sink_pin_id);
                    input_delay_nodes[sink_pin_id].set_is_from_pin_q_pin(false);
                    input_delay_nodes[sink_pin_id].set_from_pin_id(driver_pin_id);
                }
            }
        }                
    }
}

std::unordered_set<int> Timer::get_affected_cells_id_by_affected_d_pins_id(){
    std::unordered_set<int> affected_cells_id_set;    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int d_pin_id : affected_d_pins_id_set){
        const circuit::Pin &d_pin = netlist.get_pin(d_pin_id);
        int cell_id = d_pin.get_cell_id();
        affected_cells_id_set.insert(cell_id);
    }
    reset_affetced_d_pins_id_set();
    return affected_cells_id_set;        
}

void Timer::update_timing(int cell_id){        
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
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
    std::cout<<"Init Propagate Start"<<std::endl;
    //input port
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::vector<circuit::Pin> &pins = original_netlist.get_pins();
    //std::cout<<"Init Propagate from input_port"<<std::endl;
    for(const auto &pin : pins){
        if(pin.is_input_port()){            
            output_delay_nodes.at(pin.get_id()).set_delay(0.0);
            output_delay_nodes.at(pin.get_id()).init_propagate_delay();
        }
    }
    const circuit::Netlist &netlist = circuit::Netlist::get_instance(); 
    // qpins
    //std::cout<<"Init Propagate from qpin"<<std::endl;
    for(auto &it : qpin_nodes){
        it.second.init();
    }   
}


void Timer::switch_to_other_solution(const std::unordered_map<int,DpinNode> &dpin_nodes,const std::unordered_map<int,QpinNode> &qpin_nodes,const std::unordered_map<int,InputDelayNode> &input_delay_nodes,const std::unordered_map<int,OutputDelayNode> &output_delay_nodes){
    this -> dpin_nodes = dpin_nodes;
    this -> qpin_nodes = qpin_nodes;
    this -> input_delay_nodes = input_delay_nodes;
    this -> output_delay_nodes = output_delay_nodes;
    
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
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    update_timing(cells_id.at(0));
    //std::cout<<"Update Timing Finish"<<std::endl;
    const std::unordered_set<int> &affected_cells_id_set = get_affected_cells_id_by_affected_d_pins_id();
    double original_tns = 0.0;
    for(int cell_id : affected_cells_id_set){
        const circuit::Cell &cell = netlist.get_cell(cell_id);        
        original_tns += cell.get_tns();
        std::cout<<"Original Cell:"<<cell_id<<" TNS:"<<cell.get_tns()<<std::endl;
    }    
    double new_tns = 0.0;
    update_cells_tns(affected_cells_id_set);
    for(int cell_id : affected_cells_id_set){
        const circuit::Cell &cell = netlist.get_cell(cell_id);   
        new_tns += cell.get_tns();        
        std::cout<<"Updated Cell:"<<cell_id<<" TNS:"<<cell.get_tns()<<std::endl;
    }

    std::cout<<"Original TNS:"<<original_tns<<" New TNS:"<<new_tns<<std::endl;

    cost_change = (new_tns - original_tns) * timing_factor;
    affected_cells_id.insert(affected_cells_id.end(),affected_cells_id_set.begin(),affected_cells_id_set.end());
    return cost_change;
}

void Timer::reset_output_delay_nodes_max_delay(){
    for(auto &it : output_delay_nodes){
        it.second.reset_max_delay();
    }
}

void Timer::update_timing_and_cells_tns(){
    reset_output_delay_nodes_max_delay();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::vector<circuit::Pin> &pins = original_netlist.get_pins();
    for(const auto &pin : pins){
        if(pin.is_input_port()){                        
            output_delay_nodes.at(pin.get_id()).propagate_delay();
        }
    }

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    for(int cell_id : netlist.get_sequential_cells_id()){
        update_timing(cell_id);
    }
    const std::unordered_set<int> &affected_cells_id_set = get_affected_cells_id_by_affected_d_pins_id();
    update_cells_tns(affected_cells_id_set);
    collect_non_critical_ffs_id();
} 

void Timer::update_timing_and_cells_tns(int cell_id){
    update_timing(cell_id);
    const std::unordered_set<int> &affected_cells_id_set = get_affected_cells_id_by_affected_d_pins_id();
    update_cells_tns(affected_cells_id_set);
}


void Timer::update_cells_tns(const std::unordered_set<int> &affected_cells_id){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int cell_id : affected_cells_id){
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        cell.calculate_tns();
    }
}

void Timer::update_critical_q_pin(){
    reset_critical_q_pin();
    reset_critical_ffs_id();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(auto &it : output_delay_nodes){        
        const OutputDelayNode &output_delay_node = it.second;        
        if(output_delay_node.is_port()){
            continue;
        }
        int max_delay_input_pin_id = output_delay_node.get_max_delay_input_pin_id();
        const InputDelayNode &input_delay_node = input_delay_nodes.at(max_delay_input_pin_id);
        if(input_delay_node.get_is_from_pin_q_pin()){
            int q_pin = input_delay_node.get_from_pin_id();
            add_critical_q_pin(q_pin);        
            int cell_id = netlist.get_pin(q_pin).get_cell_id();
            add_critical_ffs_id(cell_id);
        }
    }
}

void Timer::collect_non_critical_ffs_id(){
    reset_critical_info();
    update_critical_q_pin();
    std::cout<<"critical_ffs_set size:"<<critical_ffs_set.size()<<std::endl;
    collect_exist_non_critical_q_pin_ffs_id();
    std::cout<<"exist_non_critical_q_pin_ffs_id size:"<<exist_non_critical_q_pin_ffs_id.size()<<std::endl;
    collect_all_non_critical_q_pin_ffs_id();
    std::cout<<"all_non_critical_q_pin_ffs_id size:"<<all_non_critical_q_pin_ffs_id.size()<<std::endl;
}

void Timer::collect_exist_non_critical_q_pin_ffs_id(){                
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cell_id : sequential_cells_id){
        const circuit::Cell &cell = netlist.get_cell(cell_id);
        const std::vector<int> &output_pins_id = cell.get_output_pins_id();
        bool is_all_critical = true;
        for(int output_pin_id : output_pins_id){
            if(!is_critical_q_pin(output_pin_id)){
                is_all_critical = false;
                break;
            }
        }
        if(!is_all_critical){
            add_exist_non_critical_q_pin_ffs_id(cell_id);       
        }        
    }    
}

void Timer::collect_all_non_critical_q_pin_ffs_id(){            
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cell_id : sequential_cells_id){
        if(is_critical_ffs_id(cell_id)){
            continue;
        }
        add_all_non_critical_q_pin_ffs_id(cell_id);
    }        
}

std::vector<int> Timer::get_critical_q_pins_id(){
    update_critical_q_pin();
    std::vector<int> critical_q_pins_id;
    critical_q_pins_id.insert(critical_q_pins_id.end(),critical_q_pin_set.begin(),critical_q_pin_set.end());
    return critical_q_pins_id;
}


std::vector<int> Timer::get_sorted_critical_q_pins_id(){    
    std::unordered_map<int,double> critical_q_pins_id_delay_map;
    for(auto &it : output_delay_nodes){
        const OutputDelayNode &output_delay_node = it.second;
        if(output_delay_node.is_port()){
            continue;
        }
        int max_delay_input_pin_id = output_delay_node.get_max_delay_input_pin_id();        
        const InputDelayNode &input_delay_node = input_delay_nodes[max_delay_input_pin_id];
        if(input_delay_node.get_is_from_pin_q_pin()){
            int q_pin = input_delay_node.get_from_pin_id();            
            double delay = output_delay_node.get_delay();            
            critical_q_pins_id_delay_map[q_pin] += delay;
        }
    }
    std::vector<std::pair<int,double>> critical_q_pins_id_delay;
    for(auto &it : critical_q_pins_id_delay_map){
        critical_q_pins_id_delay.push_back(it);
    }
    std::sort(critical_q_pins_id_delay.begin(),critical_q_pins_id_delay.end(),[](const std::pair<int,double> &a,const std::pair<int,double> &b){
        return a.second > b.second;
    });
    
    std::vector<int> sorted_critical_q_pins_id;   
    for(auto &it : critical_q_pins_id_delay){        
        sorted_critical_q_pins_id.push_back(it.first);
    }

    return sorted_critical_q_pins_id;
}

std::vector<int> Timer::get_sorted_critical_ffs_id(){
    
    std::unordered_map<int,double> critical_ffs_id_delay_map;
    std::vector<std::pair<int,double>> critical_ffs_id_delay;
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();    

    for(auto &it : output_delay_nodes){
        const OutputDelayNode &output_delay_node = it.second;
        if(output_delay_node.is_port()){
            continue;
        }
        int max_delay_input_pin_id = output_delay_node.get_max_delay_input_pin_id();                    
        const InputDelayNode &input_delay_node = input_delay_nodes.at(max_delay_input_pin_id);
        if(input_delay_node.get_is_from_pin_q_pin()){
            int q_pin_id = input_delay_node.get_from_pin_id();
            double delay = output_delay_node.get_delay() * output_delay_node.get_fanout_size();
            const circuit::Pin &q_pin = netlist.get_pin(q_pin_id);
            int cell_id = q_pin.get_cell_id();
            critical_ffs_id_delay_map[cell_id] += delay;
        }
    }

    for(auto &it : critical_ffs_id_delay_map){
        critical_ffs_id_delay.push_back(it);
    }
    std::sort(critical_ffs_id_delay.begin(),critical_ffs_id_delay.end(),[](const std::pair<int,double> &a,const std::pair<int,double> &b){
        return a.second > b.second;
    });
    
    std::vector<int> sorted_critical_ffs_id;   
    for(auto &it : critical_ffs_id_delay){        
        sorted_critical_ffs_id.push_back(it.first);
    }
    return sorted_critical_ffs_id;
}

std::vector<int> Timer::get_timing_ranking_legalize_order_ffs_id(){
    std::vector<int> legalize_order_ffs_id;
    const std::vector<int> &sorted_critical_ffs_id = get_sorted_critical_ffs_id();    

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::vector<std::pair<int,double>> non_critical_ffs_id_tns;    

    for(int cell_id : netlist.get_sequential_cells_id()){
        if(!is_critical_ffs_id(cell_id)){
            const circuit::Cell &cell = netlist.get_cell(cell_id);
            double tns = cell.get_tns();
            non_critical_ffs_id_tns.push_back(std::make_pair(cell_id,tns));
        }
    }

    std::sort(non_critical_ffs_id_tns.begin(),non_critical_ffs_id_tns.end(),[](const std::pair<int,double> &a,const std::pair<int,double> &b){
        return a.second > b.second;
    });

    legalize_order_ffs_id.reserve(sorted_critical_ffs_id.size() + non_critical_ffs_id_tns.size());
    legalize_order_ffs_id.insert(legalize_order_ffs_id.end(),sorted_critical_ffs_id.begin(),sorted_critical_ffs_id.end());
    for(auto &it : non_critical_ffs_id_tns){
        legalize_order_ffs_id.push_back(it.first);
    }
    
    std::cout<<"Critical FFS ID Size:"<<sorted_critical_ffs_id.size()<<" Non Critical FFS ID Size:"<<non_critical_ffs_id_tns.size()<<std::endl;
            
    if(legalize_order_ffs_id.size() != netlist.get_sequential_cells_id().size()){
        std::cout<<"ERROR: Legalize Order FFS ID Size Not Equal to Sequential Cells ID Size"<<std::endl;
    }    
    return legalize_order_ffs_id;
}

std::pair<int,int> Timer:: get_ff_input_pin_fanin_location(int input_pin_id) const{
    const DpinNode &dpin_node = dpin_nodes.at(input_pin_id);
    std::pair<int,int> location;
    if(dpin_node.get_is_from_pin_q_pin()){
        int q_pin_id = dpin_node.get_from_pin_id();        
        const circuit::Netlist &netlist = circuit::Netlist::get_instance();
        const circuit::Pin &q_pin = netlist.get_pin(q_pin_id);
        return std::make_pair(q_pin.get_x(),q_pin.get_y());
    }else{
        int output_pin_id = dpin_node.get_from_pin_id();
        const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
        const circuit::Pin &output_pin = original_netlist.get_pin(output_pin_id);
        return std::make_pair(output_pin.get_x(),output_pin.get_y());
    }   
}

} // namespace timer