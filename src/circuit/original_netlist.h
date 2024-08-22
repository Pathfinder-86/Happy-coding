#ifndef CORIGINAL_NETLIST_H
#define CORIGINAL_NETLIST_H


#include <string>
#include <vector>
#include <unordered_map>
#include <pin.h>
#include <cell.h>
#include <net.h>
#include <stdexcept>
#include <unordered_set>
namespace circuit {

class OriginalNetlist {
public:
    static OriginalNetlist& get_instance() {
        static OriginalNetlist instance;
        return instance;
    }
public:
    void add_comb_cell(const Cell& cell) {        
        comb_cells.push_back(cell);        
    }
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
            return cells[id];
        } else {
            throw std::out_of_range("Invalid const cell ID" + std::to_string(id));
        }
    }
    const Pin& get_pin(int id) const {
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid const pin ID" + std::to_string(id));
        }
    }
    const Net& get_net(int id) const {
        if (id >= 0 && id < static_cast<int>(nets.size())) {
            return nets.at(id);
        } else {
            throw std::out_of_range("Invalid const net ID :" + std::to_string(id));
        }
    }
    Cell& get_mutable_cell(int id) {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells[id];
        } else {
            throw std::out_of_range("Invalid mutable cell ID" + std::to_string(id));
        }
    }
    Pin& get_mutable_pin(int id){
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid mutable pin ID" + std::to_string(id));
        }
    }
    Net& get_mutable_net(int id){
        if (id >= 0 && id < static_cast<int>(nets.size())) {
            return nets.at(id);
        } else {
            throw std::out_of_range("Invalid mutable net ID" + std::to_string(id));
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
    int get_cell_id(const std::string& name) const {
        if (cell_id_map.count(name) > 0) {
            return cell_id_map.at(name);
        } else {
            std::cout<<"Cell name not found:" + name<<std::endl;
            return -1;
        }
    }
    int get_pin_id(const std::string& name) const {
        if (pin_id_map.count(name) > 0) {
            return pin_id_map.at(name);
        } else {            
            std::cout<<"Pin name not found:" + name<<std::endl;
            return -1;
        }
    }
    int get_net_id(const std::string& name) const {
        if (net_id_map.count(name) > 0) {
            return net_id_map.at(name);
        } else {
            std::cout<<"Net name not found:" + name<<std::endl;
            return -1;
        }
    }
    const std::vector<std::string>& get_cell_names() const {
        return cell_names;
    }
    const std::vector<std::string>& get_net_names() const {
        return net_names;
    }
    const std::vector<std::string>& get_pin_names() const {
        return pin_names;
    }
    const std::string& get_cell_name(int id) const {
        if (id >= 0 && id < static_cast<int>(cell_names.size())) {
            return cell_names.at(id);
        } else {
            throw std::out_of_range("Invalid cell ID:" + std::to_string(id));
        }
    }
    const std::string& get_net_name(int id) const {
        if (id >= 0 && id < static_cast<int>(net_names.size())) {
            return net_names.at(id);
        } else {
            throw std::out_of_range("Invalid net ID:" + std::to_string(id));
        }
    }
    const std::string &get_pin_name(int id) const {
        if (id >= 0 && id < static_cast<int>(pin_names.size())) {
            return pin_names.at(id);
        } else {
            throw std::out_of_range("Invalid pin ID:" + std::to_string(id));
        }
    }
    int comb_cells_size() const { return comb_cells.size(); } 
    int cells_size() const { return cells.size(); }
    int pins_size() const { return pins.size(); }
    int nets_size() const { return nets.size(); }    
    // check
    bool check_cells_location(const std::vector<Cell> &);
    bool check_overlap(const std::vector<Cell> &);
    bool check_out_of_die(const std::vector<Cell> &);
    // print msg
    void print_cell_info(const circuit::Cell &cell);
    // MAPPING ff_pin at oringinal pin
    void add_ff_pin_id_to_original_pin_id(int ff_pin_id,int original_pin_id){
        ff_pin_id_to_original_pin_id[ff_pin_id] = original_pin_id;
        original_pin_id_to_ff_pin_id[original_pin_id] = ff_pin_id;
    }
    void add_ff_cell_id_to_original_cell_id(int ff_cell_id,int original_cell_id){
        ff_cell_id_to_original_cell_id[ff_cell_id] = original_cell_id;
        original_cell_id_to_ff_cell_id[original_cell_id] = ff_cell_id;
    }
    int get_original_pin_id(int ff_pin_id) const{
        if(ff_pin_id_to_original_pin_id.count(ff_pin_id)){
            return ff_pin_id_to_original_pin_id.at(ff_pin_id);
        }else{
            return -1;
        }
    }
    int get_ff_pin_id(int original_pin_id) const{
        if(original_pin_id_to_ff_pin_id.count(original_pin_id)){
            return original_pin_id_to_ff_pin_id.at(original_pin_id);
        }else{
            return -1;
        }
    }
    int get_original_cell_id(int ff_cell_id) const{
        if(ff_cell_id_to_original_cell_id.count(ff_cell_id)){
            return ff_cell_id_to_original_cell_id.at(ff_cell_id);
        }else{
            return -1;
        }
    }
    int get_ff_cell_id(int original_cell_id) const{
        if(original_cell_id_to_ff_cell_id.count(original_cell_id)){
            return original_cell_id_to_ff_cell_id.at(original_cell_id);
        }else{
            return -1;
        }
    }
    const std::unordered_map<int,int> &get_ff_pin_id_to_original_pin_id() const{
        return ff_pin_id_to_original_pin_id;
    }
    const std::unordered_map<int,int> &get_original_pin_id_to_ff_pin_id() const{
        return original_pin_id_to_ff_pin_id;
    }
    const std::unordered_map<int,int> &get_ff_cell_id_to_original_cell_id() const{
        return ff_cell_id_to_original_cell_id;
    }
    const std::unordered_map<int,int> &get_original_cell_id_to_ff_cell_id() const{
        return original_cell_id_to_ff_cell_id;
    }
    


private:    
    // comb + seq
    std::vector<Cell> comb_cells; // const for overlap check, site removal
    std::vector<Cell> cells; // const
    std::vector<Net> nets; // const
    std::vector<Pin> pins; // const
    std::vector<std::string> cell_names; // const
    std::vector<std::string> net_names; // const
    std::vector<std::string> pin_names; // const
    std::unordered_map<std::string, int> cell_id_map; // const
    std::unordered_map<std::string, int> net_id_map; // const
    std::unordered_map<std::string, int> pin_id_map; // const
    // MAPPING ff_pin at oringinal pin
    std::unordered_map<int,int> ff_pin_id_to_original_pin_id;
    std::unordered_map<int,int> original_pin_id_to_ff_pin_id;
    // MAPPING ff_cell at oringinal cell
    std::unordered_map<int,int> ff_cell_id_to_original_cell_id;
    std::unordered_map<int,int> original_cell_id_to_ff_cell_id;

private:
    OriginalNetlist() {} // Private constructor to prevent instantiation
    ~OriginalNetlist() {} // Private destructor to prevent deletion
    OriginalNetlist(const OriginalNetlist&) = delete; // Delete copy constructor
    OriginalNetlist& operator=(const OriginalNetlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CORIGINAL_NETLIST_H