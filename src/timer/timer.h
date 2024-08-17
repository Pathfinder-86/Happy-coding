#ifndef TIMER_H
#define TIMER_H
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <../circuit/net.h>
#include <../circuit/pin.h>
#include <../circuit/cell.h>
#include <../design/design.h>
namespace timer {

class TimingNode{            
    public:    
        TimingNode(){}
        TimingNode(int pin_id,double slack):pin_id(pin_id),slack(slack){}
        TimingNode(int pin_id):pin_id(pin_id),slack(0.0){}

        int get_pin_id() const{
            return pin_id;
        }
        double get_slack() const{
            return slack;
        }
        void set_slack(double slack){
            this->slack = slack;
        }
        void add_slack(double slack){
            this->slack += slack;
        }
        void set_q_pin_delay(double q_pin_delay){
            this->q_pin_delay = q_pin_delay;
        }
        void set_q_pin_output_pin_placement_delay(double q_pin_output_pin_placement_delay){
            this->q_pin_output_pin_placement_delay = q_pin_output_pin_placement_delay;
        }
        void set_input_pin_d_pin_placement_delay(double input_pin_d_pin_placement_delay){
            this->input_pin_d_pin_placement_delay = input_pin_d_pin_placement_delay;
        }
        double get_q_pin_delay() const{
            return q_pin_delay;
        }
        double get_q_pin_output_pin_placement_delay() const{
            return q_pin_output_pin_placement_delay;
        }
        double get_input_pin_d_pin_placement_delay() const{
            return input_pin_d_pin_placement_delay;
        }
        void set_q_pin_output_pin_location(const std::pair<int,int> &q_pin_output_pin_location){
            this->q_pin_output_pin_location = q_pin_output_pin_location;
        }
        void set_d_pin_input_pin_location(const std::pair<int,int> &d_pin_input_pin_location){
            this->d_pin_input_pin_location = d_pin_input_pin_location;
        }

        // itself
        void update_slack_since_cell_move();
        // move affect other pin slack
        void cell_move_update_affected_d_pin_slack();

        
        // banking affect other pin slack                        
        void libcell_change_update_affected_d_pin_slack(double);

        void add_affected_d_pin(int d_pin_id){
            affected_d_pins.insert(d_pin_id);
        }

    private:
        int pin_id;
        double slack;

        // OTHER:
        // Q pin delay
        double q_pin_delay;        
        
        // fanin sequentail Q pin location                
        std::pair<int,int> q_pin_output_pin_location; // const
        double q_pin_output_pin_placement_delay;

        // ITSELF
        // D pin fainin pin
        std::pair<int,int> d_pin_input_pin_location; // const
        double input_pin_d_pin_placement_delay;

        std::unordered_set<int> affected_d_pins;    
};

class Timer {
public:
    static Timer& get_instance(){
        static Timer timer;
        return timer;
    }

    Timer(){
        const design::Design &design = design::Design::get_instance();
        displacement_delay_factor = design.get_displacement_delay();
    }
    void init_timing(const std::unordered_map<int,double> &init_pin_slack_map);
    double get_slack(int pin_id) const {
        if(timing_nodes.find(pin_id) == timing_nodes.end()){
            return 0.0;
        }else{
            return timing_nodes.at(pin_id).get_slack();
        }                
    }
    void set_slack(int pin_id, double slack) {
        if(timing_nodes.find(pin_id) == timing_nodes.end()){
            timing_nodes.insert({pin_id,TimingNode(pin_id,slack)});
        }else{
            timing_nodes.at(pin_id).set_slack(slack);
        }
    }
    // path from q_pin to each d_pin
    void create_timing_graph();
    void dfs_from_q_pin_to_each_d_pin(int q_pin);
    int dfs_until_d_pin(int fanin_pid,int pid,const std::pair<int,int> &q_pin_output_pin_location, const double q_pin_output_pin_placement_delay,const double q_pin_delay);
    void update_slack_since_cell_move(int cell_id);
    void update_slack_since_libcell_change(int cell_id);
    // banking and debanking
    void update_timing(int cell_id);
    // all
    void update_timing();
    // mutable
    TimingNode& get_mutable_timing_node(int pin_id){
        return timing_nodes.at(pin_id);
    }
    std::vector<int> dfs_until_d_pin_using_stack(int start_q_pin_id,int q_pin_output_pin_id,const std::pair<int,int> &q_pin_output_pin_location, const double q_pin_output_pin_placement_delay, const double q_pin_delay);    
    double get_displacement_delay_factor() const{
        return displacement_delay_factor;
    }
    void switch_to_other_solution(const std::unordered_map<int,TimingNode> &timing_nodes);
    const std::unordered_map<int,TimingNode>& get_timing_nodes() const{
        return timing_nodes;
    }
private:
    // slack on each node
    std::unordered_map<int,TimingNode> timing_nodes;    
    double displacement_delay_factor;

};


}

#endif // TIMER_H