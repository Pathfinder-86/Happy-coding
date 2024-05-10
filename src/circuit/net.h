#ifndef CIRCUIT_NET_H
#define CIRCUIT_NET_H

#include <vector>

namespace circuit {
class Net {
    public:
        Net(){}
        int get_id() const { return id; }
        void set_id(int id) { this->id = id; }
        void add_pin_id(int pin_id) { pin_ids.push_back(pin_id); }
        const std::vector<int>& get_pin_ids() const { return pin_ids; }
    private:
        int id;
        std::vector<int> pin_ids;
};
}

#endif // CIRCUIT_NET_H