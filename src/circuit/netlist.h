#ifndef CIRCUIT_NETLIST_H
#define CIRCUIT_NETLIST_H


#include <string>
#include <vector>
#include <unordered_map>
#include <pin.h>
#include <cell.h>
#include <net.h>
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

    const Pin& get_pin(int id) const {
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

private:
    Netlist() {} // Private constructor to prevent instantiation
    ~Netlist() {} // Private destructor to prevent deletion
    Netlist(const Netlist&) = delete; // Delete copy constructor
    Netlist& operator=(const Netlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CIRCUIT_H