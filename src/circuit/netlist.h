#ifndef CIRCUIT_NETLIST_H
#define CIRCUIT_NETLIST_H


#include <string>
#include <vector>
#include <unordered_map>
#include <pin.h>
#include <cell.h>
#include <net.h>
#include <stdexcept>
namespace circuit {

class Netlist {
public:
    static Netlist& get_instance() {
        static Netlist instance;
        return instance;
    }
public:
    void add_cell(Cell& cell,const std::string& name) {
        int id = cells.size();
        cell.set_id(id);
        cells.push_back(cell);
        cell_id_map[name] = id;
        cell_names.push_back(name);        
    }

    void add_pin(Pin& pin,const std::string& name) {
        int id = pins.size();
        pin.set_id(id);
        pins.push_back(pin);
        pin_id_map[name] = id;
        pin_names.push_back(name);
    }

    void add_net(Net& net,const std::string& name) {
        int id = nets.size();
        net.set_id(id);
        nets.push_back(net);
        net_id_map[name] = id;
        net_names.push_back(name);
    }

    const Cell& get_cell(int id) const {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells.at(id);
        } else {
            throw std::out_of_range("Invalid cell ID");
        }
    }

    Cell& get_mutable_cell(int id) {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells.at(id);
        } else {
            throw std::out_of_range("Invalid cell ID");
        }
    }

    const Pin& get_pin(int id) const {
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid pin ID");
        }
    }

    Pin& get_mutable_pin(int id){
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid pin ID");
        }
    }

    const Net& get_net(int id) const {
        if (id >= 0 && id < static_cast<int>(nets.size())) {
            return nets.at(id);
        } else {
            throw std::out_of_range("Invalid net ID");
        }
    }

    const std::vector<Cell>& get_cells() const {
        return cells;
    }

    const std::vector<Pin>& get_pins() const {
        return pins;
    }

    const std::vector<Net>& get_nets() const {
        return nets;
    }
    std::vector<Cell> &get_mutable_cells() {
        return cells;
    }
    std::vector<Pin> &get_mutable_pins() {
        return pins;
    }
    std::vector<Net> &get_mutable_nets() {
        return nets;
    }

    int get_cell_id(const std::string& name) const {
        if (cell_id_map.count(name) > 0) {
            return cell_id_map.at(name);
        } else {
            throw std::runtime_error("Cell name not found:" + name);
        }
    }
    int get_pin_id(const std::string& name) const {
        if (pin_id_map.count(name) > 0) {
            return pin_id_map.at(name);
        } else {
            throw std::runtime_error("Pin name not found:" + name);
        }
    }
    int get_net_id(const std::string& name) const {
        if (net_id_map.count(name) > 0) {
            return net_id_map.at(name);
        } else {
            throw std::runtime_error("Net name not found:" + name);
        }
    }
    const std::string& get_cell_name(int id) const {
        if (id >= 0 && id < static_cast<int>(cell_names.size())) {
            return cell_names.at(id);
        } else {
            throw std::out_of_range("Invalid cell ID:" + std::to_string(id));
        }
    }
    const std::string& get_pin_name(int id) const {
        if (id >= 0 && id < static_cast<int>(pin_names.size())) {
            return pin_names.at(id);
        } else {
            throw std::out_of_range("Invalid pin ID:" + std::to_string(id));
        }
    }
    const std::string& get_net_name(int id) const {
        if (id >= 0 && id < static_cast<int>(net_names.size())) {
            return net_names.at(id);
        } else {
            throw std::out_of_range("Invalid net ID:" + std::to_string(id));
        }
    }
    const std::vector<std::string>& get_cell_names() const {
        return cell_names;
    }
    const std::vector<std::string>& get_pin_names() const {
        return pin_names;
    }
    const std::vector<std::string>& get_net_names() const {
        return net_names;
    }
    //void cluster_cells(int id1, int id2);
    // for mapping
    void set_original_pin_names(){
        original_pin_names = pin_names;
    }
    const std::vector<std::string>& get_original_pin_names() const {
        return original_pin_names;
    }
    int cells_size() const { return cells.size(); }
    int pins_size() const { return pins.size(); }
    int nets_size() const { return nets.size(); }

private:
    std::vector<Cell> cells;
    std::vector<Net> nets;
    std::vector<Pin> pins;
    // name
    std::unordered_map<std::string, int> cell_id_map;
    std::unordered_map<std::string, int> net_id_map;
    std::unordered_map<std::string, int> pin_id_map;
    std::vector<std::string> cell_names;
    std::vector<std::string> net_names;
    std::vector<std::string> pin_names;
    // for mapping
    std::vector<std::string> original_pin_names;

private:
    Netlist() {} // Private constructor to prevent instantiation
    ~Netlist() {} // Private destructor to prevent deletion
    Netlist(const Netlist&) = delete; // Delete copy constructor
    Netlist& operator=(const Netlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CIRCUIT_H