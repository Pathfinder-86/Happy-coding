#ifndef CIRCUIT_PIN_H
#define CIRCUIT_PIN_H
namespace circuit {
class Pin {
public:
    Pin(): id(-1), cell_id(-1), net_id(-1), x(0.0), y(0.0), offset_x(0.0), offset_y(0.0),ff_pin(false),is_port(false),is_clk(false),is_input(false),is_output(false){}
    int get_id() const { return id; }
    int get_cell_id() const { return cell_id; }
    int get_net_id() const { return net_id; }
    void set_id(int id) { this->id = id; }
    void set_cell_id(int cell_id) { this->cell_id = cell_id; }
    void set_net_id(int net_id) { this->net_id = net_id; }    
    double get_x() const { return x; }
    double get_y() const { return y; }
    void set_x(double x) { this->x = x; }
    void set_y(double y) { this->y = y; }
    double get_offset_x() const { return offset_x; }
    double get_offset_y() const { return offset_y; }
    void set_offset_x(double offset_x) { this->offset_x = offset_x; }
    void set_offset_y(double offset_y) { this->offset_y = offset_y; }

    // is_ff_pin
    bool is_ff_pin() const { return ff_pin; }
    void set_ff_pin(bool ff_pin) { this->ff_pin = ff_pin; }
    
    bool is_d_pin() const { return get_is_input() && ff_pin; }
    bool is_q_pin() const { return get_is_output() && ff_pin; }
    
    void set_is_port(bool is_port) { this->is_port = is_port; }
    bool get_is_port() const { return is_port; }
    void set_is_clk(bool is_clk) { this->is_clk = is_clk; }
    bool get_is_clk() const { return is_clk; }
    void set_is_input(bool is_input) { this->is_input = is_input; }
    bool get_is_input() const { return is_input; }
    void set_is_output(bool is_output) { this->is_output = is_output; }
    bool get_is_output() const { return is_output; }
private:
    // netlist information
    int id;
    int cell_id;
    int net_id;
    double x, y;
    double offset_x, offset_y;    
    bool ff_pin;    
    bool is_port, is_clk, is_input, is_output;
};
}
#endif // PIN_H