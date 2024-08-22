#ifndef CIRCUIT_NET_H
#define CIRCUIT_NET_H

#include <vector>
#include <iostream>
namespace circuit {
class Net {
    public:
        Net(){}
        int get_id() const { return id; }
        void set_id(int id) { this->id = id; }
        void add_pin_id(int pin_id) { pins_id.push_back(pin_id); }                
        const std::vector<int>& get_pins_id() const { return pins_id; }
        // output pin
        int get_driver_pin_id() const { 
            if (!pins_id.empty()) {
                return pins_id.at(0);
            } else {
                std::cerr << "Net " << id << " has no driver pin" << std::endl;
                return -1;
            }
        }
    private:
        int id;
        std::vector<int> pins_id;        
};
}

#endif // CIRCUIT_NET_H