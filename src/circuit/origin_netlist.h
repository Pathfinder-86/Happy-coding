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
            throw std::out_of_range("Invalid net ID :" + std::to_string(id));
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
    bool is_sequential_cell(int cell_id){
        return sequential_cells_id.find(cell_id) != sequential_cells_id.end();
    }
    const std::unordered_set<int>& get_sequential_cells_id() const {
        return sequential_cells_id;
    }
    int get_sequential_cells_num() const {
        return sequential_cells_id.size();
    }
    void remove_sequential_cell(int cell_id){
        sequential_cells_id.erase(cell_id);
    }
    void add_sequential_cell(int cell_id){
        sequential_cells_id.insert(cell_id);
    }
    // check
    bool check_cells_location();
    bool check_overlap(const std::vector<Cell> &);
    bool check_out_of_die(const std::vector<Cell> &);
    // print msg
    void print_cell_info(const circuit::Cell &cell);
    // clk groups
    void set_clk_group_id_to_ff_cell_ids(const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids){
        this->clk_group_id_to_ff_cell_ids = clk_group_id_to_ff_cell_ids;
    }
    void set_ff_cell_id_to_clk_group_id(const std::unordered_map<int,int> &ff_cell_id_to_clk_group_id){
        this->ff_cell_id_to_clk_group_id = ff_cell_id_to_clk_group_id;
    }
    void add_clk_group(const std::unordered_set<int> &clk_group){
        int clk_group_id = clk_group_id_to_ff_cell_ids.size();
        clk_group_id_to_ff_cell_ids[clk_group_id] = clk_group;
        for(auto cell_id : clk_group){
            ff_cell_id_to_clk_group_id[cell_id] = clk_group_id;
        }  
    }
    int get_clk_group_id(int cell_id){
        if(ff_cell_id_to_clk_group_id.count(cell_id)){
            return ff_cell_id_to_clk_group_id[cell_id];
        }else{
            return -1;
        }
    }
    void remove_cell_from_clk_group(int cell_id){
        if(ff_cell_id_to_clk_group_id.count(cell_id)){
            int clk_group_id = ff_cell_id_to_clk_group_id[cell_id];
            ff_cell_id_to_clk_group_id.erase(cell_id);
            clk_group_id_to_ff_cell_ids[clk_group_id].erase(cell_id);
        }
    }
    void add_cell_to_clk_group(int cell_id,int clk_group_id){
        ff_cell_id_to_clk_group_id[cell_id] = clk_group_id;
        clk_group_id_to_ff_cell_ids[clk_group_id].insert(cell_id);
    }
    const std::unordered_map<int,std::unordered_set<int>> &get_clk_group_id_to_ff_cell_ids() const{
        return clk_group_id_to_ff_cell_ids;
    }            
    const std::unordered_map<int,int> &get_ff_cell_id_to_clk_group_id() const{
        return ff_cell_id_to_clk_group_id;
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
    // seq
    std::unordered_set<int> sequential_cells_id;
    // clk groups
    std::unordered_map<int,std::unordered_set<int>> clk_group_id_to_ff_cell_ids;
    std::unordered_map<int,int> ff_cell_id_to_clk_group_id;
    // MAPPING ff_pin at oringinal pin
    std::unordered_map<int,int> ff_pin_id_to_original_pin_id;

private:
    OriginalNetlist() {} // Private constructor to prevent instantiation
    ~OriginalNetlist() {} // Private destructor to prevent deletion
    OriginalNetlist(const OriginalNetlist&) = delete; // Delete copy constructor
    OriginalNetlist& operator=(const OriginalNetlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CORIGINAL_NETLIST_H