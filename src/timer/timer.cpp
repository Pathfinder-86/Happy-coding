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
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::string &output_pin_name = original_netlist.get_pin_name(id);
    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){           
        const std::string &input_pin_name = original_netlist.get_pin_name(fanout_input_delay_node_id);
        //std::cout<<"PROPAGATE: "<<output_pin_name<<" to input_pin"<<input_pin_name<<" delay: "<<get_delay()<<std::endl;
        timer.get_mutable_input_delay_node(fanout_input_delay_node_id).update( get_delay() );
    }

    for(int d_pin_node_id : d_pin_nodes_id){            
        int original_d_pin_id = original_netlist.get_original_pin_id(d_pin_node_id);
        const std::string &d_pin_name = original_netlist.get_pin_name(original_d_pin_id);
        //std::cout<<"PROPAGATE: "<<output_pin_name<<" to d_pin"<<d_pin_name<<" delay: "<<get_delay()<<std::endl;

        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
}

void OutputDelayNode::update(double delay, int input_pin_id){
    timer::Timer &timer = Timer::get_instance();
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const std::string &output_pin_name = original_netlist.get_pin_name(id);
    if(delay > get_delay()){        
        const std::string &input_pin_name = original_netlist.get_pin_name(input_pin_id);
        //std::cout<<"UPDATE: "<<output_pin_name<<" from input_pin"<<input_pin_name<<" delay: "<<delay<<std::endl;

        set_delay(delay);
        set_max_delay_input_pin_id(input_pin_id);
        propagate_delay();
        return;
    }else{
        double max_delay = get_delay();
        int max_input_pin_id = get_max_delay_input_pin_id();
        const std::string &max_input_pin_name = original_netlist.get_pin_name(max_input_pin_id);
        const std::string &input_pin_name = original_netlist.get_pin_name(input_pin_id);
        //std::cout<<"UPDATE: "<<output_pin_name<<" from input_pin"<<input_pin_name<<" delay: "<<delay<<" less than "<<max_input_pin_name<<" delay: "<<max_delay<<std::endl;
    }

    if(input_pin_id == get_max_delay_input_pin_id() && delay < get_delay()){
        //std::cout<<"UPDATE: Find new max delay input pin"<<std::endl;
        double new_delay = 0.0;
        int new_input_pin_id = -1;
        for(int input_delay_node_id : input_delay_nodes_id){           
            double input_node_delay = timer.get_input_delay_node(input_delay_node_id).get_delay();
            if(input_node_delay >= new_delay){
                new_delay = input_node_delay;
                new_input_pin_id = input_delay_node_id;
            }
        }
        const std::string &new_input_pin_name = original_netlist.get_pin_name(new_input_pin_id);
        //std::cout<<"UPDATE: "<<output_pin_name<<" from new_input_pin"<<new_input_pin_name<<" delay: "<<delay<<std::endl;        
        
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
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    const circuit::Pin &q_pin = netlist.get_pin(id);
    int q_pin_cell_id = q_pin.get_cell_id();
    int origin_q_pin_id = original_netlist.get_original_pin_id(q_pin_cell_id);
    int origin_q_pin_cell_id = original_netlist.get_original_cell_id(q_pin_cell_id);
    const std::string &q_pin_name = original_netlist.get_pin_name(origin_q_pin_id);
    const std::string &q_pin_cell_name = original_netlist.get_cell_name(origin_q_pin_cell_id);

    //std::cout<<"QPIN:"<<q_pin_cell_name<<" pin:"<<q_pin_name<<" propagate to: "<<std::endl;

    for(int fanout_input_delay_node_id : fanout_input_delay_nodes_id){      
        const std::string &fanout_input_delay_node_name = original_netlist.get_pin_name(fanout_input_delay_node_id);
        //std::cout<<"Input_pin:"<<fanout_input_delay_node_name<<std::endl;

        timer.get_mutable_input_delay_node(fanout_input_delay_node_id). update( get_delay());
    }   
    for(int d_pin_node_id : d_pin_nodes_id){            
        int origin_d_pin_id = original_netlist.get_original_pin_id(d_pin_node_id);        
        const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);
        //std::cout<<"D_pin:"<<d_pin_name<<std::endl;     

        timer.get_mutable_dpin_node(d_pin_node_id).update( get_delay() );
    }
    //std::cout<<"-------------------"<<std::endl;
}

void QpinNode::update(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const circuit::Pin &pin = netlist.get_pin(id);
    const circuit::Cell &cell = netlist.get_cell(pin.get_cell_id());    
    
    const circuit::OriginalNetlist &original_netlist = circuit::OriginalNetlist::get_instance();
    int origin_q_pin_id = original_netlist.get_original_pin_id(id);
    const std::string &q_pin_name = original_netlist.get_pin_name(origin_q_pin_id);
    //std::cout<<"q_pin: "<<q_pin_name<<" propagate_delay"<<std::endl;

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
        int origin_d_pin_id = original_netlist.get_original_pin_id(d_pin_id);
        const std::string &d_pin_name = original_netlist.get_pin_name(origin_d_pin_id);    
        //std::cout<<"----------DPIN:"<<d_pin_name<<" LOCATION CHANGE START---------"<<std::endl;                      
        dpin_nodes[d_pin_id].update_location();   
        //std::cout<<"----------DPIN LOCATION CHANGE FINISH---------"<<std::endl;     
    }    

    // propagate delay
    for(int q_pin_id : cell.get_output_pins_id()){    
        int origin_q_pin_id = original_netlist.get_original_pin_id(q_pin_id);
        const std::string &q_pin_name = original_netlist.get_pin_name(origin_q_pin_id);
        //std::cout<<"----------QPIN:"<<q_pin_name<<" PROPAGATE START---------"<<std::endl;    
        qpin_nodes[q_pin_id].update();
        //std::cout<<"-------QPIN PROPAGATE FINISH----------"<<std::endl;
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
    update_cells_slack(affected_cells_id_set);
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

void Timer::update_cells_slack(const std::unordered_set<int> &affected_cells_id){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    for(int cell_id : affected_cells_id){
        circuit::Cell &cell = netlist.get_mutable_cell(cell_id);
        cell.calculate_tns();
    }
}

} // namespace timer