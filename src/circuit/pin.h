#ifndef CIRCUIT_PIN_H
#define CIRCUIT_PIN_H
namespace circuit {
class Pin {
public:
    Pin(){ init(); };    
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
    double get_slack() const { return slack; }
    void set_slack(double slack) { this->slack = slack; }
    void init(){
        id = -1;
        cell_id = -1;
        net_id = -1;
        x = 0.0;
        y = 0.0;
        offset_x = 0.0;
        offset_y = 0.0;
        slack = 0.0;    
    }
private:
    int id;
    int cell_id;
    int net_id;
    double x, y;
    double offset_x, offset_y;
    double slack;
};
}
#endif // PIN_H