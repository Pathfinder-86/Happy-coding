#ifndef CIRCUIT_PIN_H
#define CIRCUIT_PIN_H
namespace circuit {
class Pin {
public:
    Pin(): id(-1), cell_id(-1), net_id(-1), x(0.0), y(0.0), offset_x(0.0), offset_y(0.0),is_slack_related(false) {}
    int get_id() const { return id; }
    int get_cell_id() const { return cell_id; }
    int get_net_id() const { return net_id; }
    void set_id(int id) { this->id = id; }
    void set_cell_id(int cell_id) { this->cell_id = cell_id; }
    void set_net_id(int net_id) { this->net_id = net_id; }
    bool is_port(){
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

    // slack related, FF D,Q pins, directly connect with FF D,Q pins
    bool is_slack_related() const { return is_slack_related; }
    void set_slack_related(bool is_slack_related) { this->is_slack_related = is_slack_related; }

    // 0: input, 1: output, 2: clk
    bool is_input() { return pin_connection_type == 0; }
    bool is_output() { return pin_connection_type == 1; }
    bool is_clk() { return pin_connection_type == 2; }
    void set_pin_connection_type(int pin_connection_type) { this->pin_connection_type = pin_connection_type; }

private:
    // netlist information
    int id;
    int cell_id;
    int net_id;
    double x, y;
    double offset_x, offset_y;
    // slack related
    bool is_slack_related;
    // traverse netlist
    int pin_connection_type; // 0: input, 1: output, 2: clk
};
}
#endif // PIN_H