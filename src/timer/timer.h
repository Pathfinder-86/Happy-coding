#ifndef TIMER_H
#define TIMER_H
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <../circuit/net.h>
#include <../circuit/pin.h>
#include <../circuit/cell.h>
#include <../design/design.h>
#include <../circuit/original_netlist.h>
#include <cmath>
#include <../design/libcell.h>
namespace timer {

class DpinNode;
class QpinNode;
class OutputDelayNode;
class InputDelayNode;

class OutputDelayNode {
public:
    OutputDelayNode(){}
    OutputDelayNode(int id):delay(0.0),id(id),max_delay_input_pin_id(-1){}
    int get_id() const{
        return id;
    }
    double get_delay() const{
        return delay;
    }
    void set_delay(double delay){
        this -> delay = delay;
    }
    int get_max_delay_input_pin_id() const{
        return max_delay_input_pin_id;
    }
    void set_max_delay_input_pin_id(int max_delay_input_pin_id){
        this -> max_delay_input_pin_id = max_delay_input_pin_id;
    }
    bool is_port() const{
        return max_delay_input_pin_id == -1;
    }
    void update(double delay, int input_pin_id);
    void propagate_delay();
    void init(double delay, int input_pin_id);
    void init_propagate_delay();    
    void add_fanout_input_delay_node_id(int fanout_input_delay_node_id){
        fanout_input_delay_nodes_id.push_back(fanout_input_delay_node_id);
    }
    void add_input_delay_node_id(int input_delay_node_id){
        input_delay_nodes_id.push_back(input_delay_node_id);
    }
    void add_d_pin_node(int d_pin_node_id){
        d_pin_nodes_id.push_back(d_pin_node_id);
    }
    void reset_max_delay(){
        delay = 0.0;
        max_delay_input_pin_id = -1;    
    }    
    int get_fanout_size() const {
        return fanout_input_delay_nodes_id.size() + d_pin_nodes_id.size();
    }
private:
    int id;
    double delay;
    int max_delay_input_pin_id;
    std::vector<int> input_delay_nodes_id;

    // to: using origin netlist to init
    std::vector<int> fanout_input_delay_nodes_id;
    std::vector<int> d_pin_nodes_id;
};

class InputDelayNode {
public:
    InputDelayNode(){}
    InputDelayNode(int id):id(id),init_delay(0.0),propagation_delay(0.0),is_from_pin_q_pin(false),from_pin_id(-1){}
    int get_id() const{
        return id;
    }
    void set_is_from_pin_q_pin(bool is_q_pin){
        this -> is_from_pin_q_pin = is_q_pin;
    }
    bool get_is_from_pin_q_pin() const{
        return is_from_pin_q_pin;
    }
    void set_from_pin_id(int from_pin_id){
        this -> from_pin_id = from_pin_id;
    }
    int get_from_pin_id() const{
        return from_pin_id;
    }
    double get_delay() const{
        return init_delay + propagation_delay;
    }
    void update_propagation_delay(double delay){
        this -> propagation_delay = delay;
    }
    void update_init_delay(double delay){
        this -> init_delay = delay;
    }
    void update_to_pins_delay();  
    void init_to_pins_delay();  
    void init(double init_delay);
    void update(double init_delay);
    void add_to_pins_id(int to_pin_id){
        to_pins_id.push_back(to_pin_id);
    }
private:
    int id;
    double propagation_delay; 
    double init_delay;
    
    bool is_from_pin_q_pin;
    int from_pin_id;
    // // to: using origin netlist CELL to init
    std::vector<int> to_pins_id; //output_delay_nodes
};


class QpinNode{
public:    
    QpinNode(){}
    QpinNode(int id):id(id),delay(0.0){}
    int get_id() const{
        return id;
    }
    double get_delay() const{
        return delay;
    }
    // new q_pin delay
    void set_delay(double delay){
        this -> delay = delay;
    }
    void add_fanout_input_delay_node_id(int input_delay_node_id){
        fanout_input_delay_nodes_id.push_back(input_delay_node_id);
    }
    void add_d_pin_node(int d_pin_node_id){
        d_pin_nodes_id.push_back(d_pin_node_id);
    }
    void init();
    void init_propagate_delay();
    void update();
    void propagate_delay(); 
    const std::vector<int>& get_fanout_input_delay_nodes_id() const{
        return fanout_input_delay_nodes_id;
    }
    const std::vector<int>& get_d_pin_nodes_id() const{
        return d_pin_nodes_id;
    }    
private:
    int id;
    double delay;
    // to: using origin netlist NET to init
    std::vector<int> fanout_input_delay_nodes_id;
    std::vector<int> d_pin_nodes_id;
};



class DpinNode{            
    public:    
        DpinNode(){}
        DpinNode(int id):id(id),slack(0.0),propagation_delay(0.0),init_delay(0.0),is_from_pin_q_pin(false),from_pin_id(-1){}
        int get_id() const{
            return id;
        }
        double get_slack() const{
            return slack;
        }
        void add_slack(double slack){
            this -> slack += slack;
        }
        void set_slack(double slack){
            this->slack = slack;
        }
        void set_is_from_pin_q_pin(bool is_q_pin){
            this -> is_from_pin_q_pin = is_q_pin;
        }
        bool get_is_from_pin_q_pin() const{
            return is_from_pin_q_pin;
        }
        void init(double delay);
        void update(double delay);
        void update_propagation_delay(double delay);
        void update_init_delay(double delay);
        void update_location();        
        void set_from_pin_id(int from_pin_id){
            this -> from_pin_id = from_pin_id;
        }
        int get_from_pin_id() const{
            return from_pin_id;
        }
        void set_init_delay(double delay){
            this -> init_delay = delay;
        }
        void set_propagation_delay(double delay){
            this -> propagation_delay = delay;
        }
        double get_propagation_delay() const{
            return propagation_delay;
        }
    private:
        int id;
        double slack;
        double propagation_delay; 
        double init_delay;        
    
        bool is_from_pin_q_pin;
        int from_pin_id;        
};

class Timer {
public:
    static Timer& get_instance(){
        static Timer timer;
        return timer;
    }

    Timer(){}
    // init d pin slack
    void init_propagate();
    void init_timing(const std::unordered_map<int,double> &init_pin_slack_map);
    // get/set slack
    double get_slack(int id) const {
        return dpin_nodes.at(id).get_slack();     
    }
    void set_slack(int id, double slack) {
        dpin_nodes.at(id).set_slack(slack);
    }
    // banking and debanking
    void update_timing(int cell_id);
    void update_timing_by_cells_id(const std::vector<int> &cells_id);
    void update_timing();
    // init
    // path from q_pin to each d_pin
    void create_timing_graph();        
    double get_displacement_delay_factor() const{
        const design::Design &design = design::Design::get_instance();
        return design.get_displacement_delay();
    }

    // fast timer
    void write_init_timing_data();
    void create_timing_graph_by_read_data();

    // cost
    double calculate_timing_cost_after_cluster(const std::vector<int> &cells_id, std::vector<int> &affected_cells_id);

    // get_nodes
    const std::unordered_map<int,DpinNode> get_dpin_nodes() const{
        return dpin_nodes;
    }
    const QpinNode& get_qpin_node(int id) const{
        return qpin_nodes.at(id);
    }
    QpinNode& get_mutable_qpin_node(int id){
        return qpin_nodes.at(id);
    }
    const std::unordered_map<int,QpinNode> get_qpin_nodes() const{
        return qpin_nodes;
    }
    const DpinNode& get_dpin_node(int id) const{
        return dpin_nodes.at(id);
    }
    DpinNode& get_mutable_dpin_node(int id){
        return dpin_nodes.at(id);
    }
    const std::unordered_map<int,OutputDelayNode> get_output_delay_nodes() const{
        return output_delay_nodes;
    }
    const std::unordered_map<int,InputDelayNode> get_input_delay_nodes() const{
        return input_delay_nodes;
    }
    std::unordered_map<int,OutputDelayNode>& get_mutable_output_delay_nodes(){
        return output_delay_nodes;
    }
    std::unordered_map<int,InputDelayNode>& get_mutable_input_delay_nodes(){
        return input_delay_nodes;
    }
    OutputDelayNode& get_mutable_output_delay_node(int id){
        return output_delay_nodes.at(id);
    }
    InputDelayNode& get_mutable_input_delay_node(int id){
        return input_delay_nodes.at(id);
    }
    const OutputDelayNode& get_output_delay_node(int id) const{
        return output_delay_nodes.at(id);
    }
    const InputDelayNode& get_input_delay_node(int id) const{
        return input_delay_nodes.at(id);
    }
    void traverse_each_net_to_add_connection();
    void init_q_pin_delay();    
    void switch_to_other_solution(const std::unordered_map<int,DpinNode> &dpin_nodes,const std::unordered_map<int,QpinNode> &qpin_nodes,const std::unordered_map<int,InputDelayNode> &input_delay_nodes,const std::unordered_map<int,OutputDelayNode> &output_delay_nodes);
    void update_cells_tns(const std::unordered_set<int> &affected_cells_id_set);    
    std::unordered_set<int> get_affected_cells_id_by_affected_d_pins_id();
    void reset_affetced_d_pins_id_set(){
        affected_d_pins_id_set.clear();
    }
    void add_affected_d_pin_id(int d_pin_id){
        affected_d_pins_id_set.insert(d_pin_id);
    }
    void add_intput_delay_node_to_pins_id();

    void update_timing_and_cells_tns(int cell_id);
    void reset_output_delay_nodes_max_delay();
    void update_timing_and_cells_tns();
        
    void collect_all_non_critical_q_pin_ffs_id();
    void collect_exist_non_critical_q_pin_ffs_id();

    std::vector<int> get_critical_q_pins_id();
    std::vector<int> get_sorted_critical_q_pins_id();
    std::vector<int> get_sorted_critical_ffs_id();
    std::vector<int> get_timing_ranking_legalize_order_ffs_id();
    std::pair<int,int> get_ff_input_pin_fanin_location(int d_pin_id) const;
    bool is_critical_q_pin(int q_pin_id) const{
        return critical_q_pin_set.find(q_pin_id) != critical_q_pin_set.end();
    }
    void add_critical_q_pin(int q_pin_id){
        critical_q_pin_set.insert(q_pin_id);
    }
    void reset_critical_q_pin(){
        critical_q_pin_set.clear();
    }
    const std::unordered_set<int>& get_critical_q_pin_set() const{
        return critical_q_pin_set;
    }
    void update_critical_q_pin();
    void add_critical_ffs_id(int cell_id){
        critical_ffs_set.insert(cell_id);
    }
    void reset_critical_ffs_id(){
        critical_ffs_set.clear();
    }
    const std::unordered_set<int>& get_critical_ffs_id() const{
        return critical_ffs_set;
    }
    bool is_critical_ffs_id(int cell_id) const{
        return critical_ffs_set.find(cell_id) != critical_ffs_set.end();
    }
    void collect_non_critical_ffs_id();
    void add_exist_non_critical_q_pin_ffs_id(int cell_id){
        exist_non_critical_q_pin_ffs_id.insert(cell_id);
    }
    void add_all_non_critical_q_pin_ffs_id(int cell_id){
        all_non_critical_q_pin_ffs_id.insert(cell_id);
    }
    void reset_all_non_critical_q_pin_ffs_id(){
        all_non_critical_q_pin_ffs_id.clear();
    }
    void reset_exist_non_critical_q_pin_ffs_id(){
        exist_non_critical_q_pin_ffs_id.clear();
    }
    void reset_critical_info(){
        reset_critical_q_pin();
        reset_critical_ffs_id();
        reset_all_non_critical_q_pin_ffs_id();
        reset_exist_non_critical_q_pin_ffs_id();   
    }
    const std::unordered_set<int>& get_exist_non_critical_q_pin_ffs_id() const{
        return exist_non_critical_q_pin_ffs_id;
    }
    const std::unordered_set<int>& get_all_non_critical_q_pin_ffs_id() const{
        return all_non_critical_q_pin_ffs_id;
    }
private:
    // slack on each node
    std::unordered_map<int,DpinNode> dpin_nodes;
    std::unordered_map<int,QpinNode> qpin_nodes;
    std::unordered_map<int,InputDelayNode> input_delay_nodes;
    std::unordered_map<int,OutputDelayNode> output_delay_nodes;
    std::unordered_set<int> affected_d_pins_id_set;
    
    // changeable: call collect_non_critical_ffs_id to update
    std::unordered_set<int> critical_q_pin_set;
    std::unordered_set<int> critical_ffs_set;
    std::unordered_set<int> exist_non_critical_q_pin_ffs_id;
    std::unordered_set<int> all_non_critical_q_pin_ffs_id;

};


}

#endif // TIMER_H