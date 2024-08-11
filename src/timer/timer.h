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
        // id = pid
        TimingNode(){}
        TimingNode(int id,double slack):id(id),slack(slack){}
        TimingNode(int id):id(id),slack(0.0){}
        void slack_change(double delay){
            this->slack += delay;
        }
        int get_id() const{
            return id;
        }
        double get_slack() const{
            return slack;
        }
        void set_slack(double slack){
            this->slack = slack;
        }
    private:
        int id;
        double slack;        
};

class TimingEdge{
    public:
        TimingEdge(){}
        TimingEdge(double delay):delay(delay){}
        double get_delay(){
            return delay;
        }
        void set_delay(double delay){
            this->delay = delay;
        }        
    private:
        double delay = 0.0;
};

class Timer {
public:
    static Timer& get_instance(){
        static Timer timer;
        return timer;
    }

    Timer():dirty(true) {}
    void update_timing();
    void add_net_into_timing_graph(const circuit::Net& net);
    void add_cell_delay_into_timing_graph(const circuit::Cell &cell);
    void init_timing(const std::unordered_map<int,double> &init_pin_slack_map);
    void set_dirty(bool dirty) {
        this->dirty = dirty;
    }
    bool is_dirty() const {
        return dirty;
    }
    double get_slack(int pin_id) const {
        if(timing_nodes.find(pin_id) == timing_nodes.end()){
            return 0.0;
        }else{
            return timing_nodes.at(pin_id).get_slack();
        }                
    }
    void set_slack(int pin_id, double slack) {
        timing_nodes[pin_id].set_slack(slack);
    }

private:
    // driver_pin_id -> sink_pin_ids
    std::unordered_map<int,std::unordered_set<int>> timing_graph;
    // slack on each node
    std::unordered_map<int,TimingNode> timing_nodes;
    // edge:from, to  -> delay
    std::unordered_map<int,std::unordered_map<int,TimingEdge>> timing_edges;
    // is timer dirty
    bool dirty;
    
};


}

#endif // TIMER_H