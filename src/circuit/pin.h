#ifndef CIRCUIT_PIN_H
#define CIRCUIT_PIN_H
namespace circuit {
class Pin {
public:
    Pin(): id(-1), cell_id(-1), net_id(-1), x(0.0), y(0.0), offset_x(0.0), offset_y(0.0),ff_pin(false),pin_connection_type(-1) {}
    int get_id() const { return id; }
    int get_cell_id() const { return cell_id; }
    int get_net_id() const { return net_id; }
    void set_id(int id) { this->id = id; }
    void set_cell_id(int cell_id) { this->cell_id = cell_id; }
    void set_net_id(int net_id) { this->net_id = net_id; }
    bool is_port() const{
        return cell_id == -1;
    }
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
    
    bool is_d_pin() const { return is_input() && ff_pin; }
    
    // 0: input, 1: output, 2: clk
    bool is_input() const { return pin_connection_type == 0; }   // sink: 
    bool is_output() const { return pin_connection_type == 1; }  // source: driver pin of a net
    bool is_other() const { return pin_connection_type != 0 && pin_connection_type != 1; } // clk or other
    void set_pin_connection_type(int pin_connection_type) { this->pin_connection_type = pin_connection_type; }
    
    bool no_connection() const { return net_id == -1; }

private:
    // netlist information
    int id;
    int cell_id;
    int net_id;
    double x, y;
    double offset_x, offset_y;    
    bool ff_pin;
    // traverse netlist
    int pin_connection_type; // 0: input, 1: output, else clk or other
};
}
#endif // PIN_H