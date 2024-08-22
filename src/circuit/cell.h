#ifndef CIRCUIT_CELL_H
#define CIRCUIT_CELL_H

#include <vector>
namespace circuit {
class Pin;
class Cell {
private:
    std::vector<int> input_pins_id;
    std::vector<int> output_pins_id;
    int other_pin_id;
    int x, y;
    int w, h;
    int id;
    int lib_cell_id;
    int parent;
    double slack;
    bool sequential;
public:
    Cell(): x(0), y(0), w(0), h(0), id(-1), lib_cell_id(-1), parent(-1), slack(0.0), sequential(false), other_pin_id(-1) {}
    Cell(int x, int y, int w, int h) : x(x), y(y), w(w), h(h), id(-1), lib_cell_id(-1), parent(-1), slack(0.0), sequential(false), other_pin_id(-1) {}
    int get_x() const { return x; }
    int get_y() const { return y; }
    int get_w() const { return w; }
    int get_h() const { return h; }
    int get_rx() const { return x + w; }
    int get_ry() const { return y + h; }
    int get_id() const { return id; }
    void set_x(int x) { this->x = x; }
    void set_y(int y) { this->y = y; }
    void set_w(int w) { this->w = w; }
    void set_h(int h) { this->h = h; }
    void set_id(int id) { this->id = id; }    
    std::vector<int> get_pins_id() const{
        std::vector<int> all_pins_id;
        all_pins_id.insert(all_pins_id.end(), input_pins_id.begin(), input_pins_id.end());
        all_pins_id.insert(all_pins_id.end(), output_pins_id.begin(), output_pins_id.end());
        if(other_pin_id != -1){
            all_pins_id.push_back(other_pin_id);
        }
        return all_pins_id;
    }
    void add_input_pin_id(int pin_id) { input_pins_id.push_back(pin_id); }
    void add_output_pin_id(int pin_id) { output_pins_id.push_back(pin_id); }
    void set_input_pins_id(const std::vector<int>& input_pins_id) { this->input_pins_id = input_pins_id; }
    void set_output_pins_id(const std::vector<int>& output_pins_id) { this->output_pins_id = output_pins_id; }
    const std::vector<int>& get_input_pins_id() const { return input_pins_id; }
    const std::vector<int>& get_output_pins_id() const { return output_pins_id; }
    void set_other_pin_id(int other_pin_id) { this->other_pin_id = other_pin_id; }
    int get_other_pin_id() const { return other_pin_id; }
    void set_lib_cell_id(int lib_cell_id) { this->lib_cell_id = lib_cell_id; }
    int get_lib_cell_id() const { return lib_cell_id; }
    void move(int x, int y);
    // ff
    void set_sequential(bool sequential) { this->sequential = sequential; }
    bool is_sequential() const { return sequential; }    
    // cluster
    void set_parent(int parent) { this->parent = parent; }
    int get_parent() const { return parent; }
    bool is_clustered() const { return parent != -1; }
    // slack
    double get_delay() const;
    void calculate_slack();
    void set_slack(double slack) { this->slack = slack; }
    double get_slack() const { return slack; }
    // area
    double get_area() const { return w * h; }
    // power
    double get_power() const;
    // clear: if cell been clustered, consider it as a empty point
    void clear(){
        input_pins_id.clear();
        output_pins_id.clear();
        x = 0;
        y = 0;
        w = 0;
        h = 0;
        lib_cell_id = -1;   
        slack = 0.0;                
    }
    // overlap 
    bool overlap(const Cell& cell) const;
    // bits
    int get_bits() const {
        return input_pins_id.size();
    }
};
}
#endif // CELL_H