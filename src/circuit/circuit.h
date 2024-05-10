#ifndef CIRCUIT_H
#define CIRCUIT_H


#include <string>
#include <vector>
#include <map>
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
        void add_cell(Cell& cell) {
            cell.set_id(cells.size());
            cells.push_back(cell);
        }

        void add_pin(Pin& pin) {
            pin.set_id(pins.size());
            pins.push_back(pin);
        }

        void add_net(Net& net) {
            net.set_id(nets.size());
            nets.push_back(net);
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

private:
    Netlist() {} // Private constructor to prevent instantiation
    ~Netlist() {} // Private destructor to prevent deletion
    Netlist(const Netlist&) = delete; // Delete copy constructor
    Netlist& operator=(const Netlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CIRCUIT_H