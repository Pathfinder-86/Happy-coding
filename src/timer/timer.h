#ifndef TIMER_H
#define TIMER_H

namespace timer {

class TimingNode{
    public:
        TimingNode(int id,double slack):id(id),slack(slack){}
        TimingNode(int id):id(id),slack(0.0){}
        set_dirty(bool dirty){
            this->dirty = dirty;
        }
        bool is_dirty(){
            return dirty;
        }
        void slack_change(double delay){
            this->slack += delay;
        }
        int get_id(){
            return id;
        }
    private:
        int id;
        double slack;
        bool dirty = false;
}

class TimingEdge{
    public:
        TimingEdge(int from,int to,double delay):from(from),to(to),delay(delay){}
        double get_delay(){
            return delay;
        }
        void set_delay(double delay){
            this->delay = delay;
        }
    private:
        // use from to as key
        int from,to;
        double delay = 0.0;
}

class Timer {
public:
    static Timer& get_instance() {
        static Timer timer;
        return timer;
    }

    Timer() {}
    void add_node(const TimingNode& node) {
        int id = node.get_id();
        graph.insert({id,std::unordered_set<int>()});
        nodes[id] = node;
    }
    void update_timing();
    
    
private:
    std::unordered_map<int,std::unordered_set<int>> timing_graph;
    std::unordered_map<int,TimingNode> nodes;
    std::unordered_map<int,std::unordered_map<int,TimingEdge>> edges;    
};


}

#endif // TIMER_H