#ifndef CIRCUIT_PIN_H
#define CIRCUIT_PIN_H
namespace circuit {
class Pin {
public:
    Pin(){ cell_id = -1; net_id = -1;};
    Pin(int cell_id, int net_id) : cell_id(cell_id), net_id(net_id) {}
    int get_id() const { return id; }
    int get_cell_id() const { return cell_id; }
    int get_net_id() const { return net_id; }
    void set_id(int id) { this->id = id; }
    void set_cell_id(int cell_id) { this->cell_id = cell_id; }
    void set_net_id(int net_id) { this->net_id = net_id; }
    bool is_port(){
        return cell_id == -1;
    }
private:
    int id;
    int cell_id;
    int net_id;
};
}
#endif // PIN_H