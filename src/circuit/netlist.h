#ifndef CIRCUIT_NETLIST_H
#define CIRCUIT_NETLIST_H


#include <string>
#include <vector>
#include <unordered_map>
#include <pin.h>
#include <cell.h>
#include <stdexcept>
#include <unordered_set>
namespace circuit {

class Netlist {
public:
    static Netlist& get_instance() {
        static Netlist instance;
        return instance;
    }
public:
    void add_cell(Cell& cell) {
        int id = cells.size();
        cell.set_id(id);
        cells.push_back(cell);
    }

    void add_pin(Pin& pin) {
        int id = pins.size();
        pin.set_id(id);
        pins.push_back(pin);
    }

    const Cell& get_cell(int id) const {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells[id];
        } else {
            throw std::out_of_range("Invalid const cell ID" + std::to_string(id));
        }
    }

    int get_cell_bits(int id) const {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells[id].get_bits();
        } else {
            throw std::out_of_range("Invalid const cell ID" + std::to_string(id));
        }
    }

    Cell& get_mutable_cell(int id) {
        if (id >= 0 && id < static_cast<int>(cells.size())) {
            return cells[id];
        } else {
            throw std::out_of_range("Invalid mutable cell ID " + std::to_string(id));
        }
    }

    const Pin& get_pin(int id) const {
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid const pin ID" + std::to_string(id));
        }
    }

    Pin& get_mutable_pin(int id){
        if (id >= 0 && id < static_cast<int>(pins.size())) {
            return pins.at(id);
        } else {
            throw std::out_of_range("Invalid mutable pin ID" + std::to_string(id));
        }
    }
    const std::vector<Cell>& get_cells() const {
        return cells;
    }
    std::vector<Cell>& get_mutable_cells() {
        return cells;
    }

    const std::vector<Pin>& get_pins() const {
        return pins;
    }
    int cells_size() const { return cells.size(); }
    int pins_size() const { return pins.size(); }
    
    // libcell mapping
    // return 0: success , return 1: legalize fail, return 2: no ff, return 3: others
    int try_legal_and_modify_circuit_since_merge_cell(const std::vector<int> &cells_id,const int new_lib_cell_id);
    int try_legal_and_modify_circuit_since_merge_cell_skip_timer(const std::vector<int> &cells_id,const int new_lib_cell_id);
    int modify_circuit_since_divide_cell(int c,const int new_lib_cell_id1,const int new_lib_cell_id2);

    // return 0: success , return 1: legalize fail, return 2: no ff, return 3: others
    int cluster_cells(const std::vector<int> &cell_ids); 
    int cluster_cells_without_check(const std::vector<int> &cell_ids); 
    // return 0: success , return 1: legalize fail, return 2: no ff target to decluster,return 3: no ff for decluster, return 4: others
    int decluster_cells(int cid);    

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
    // set_cells 
    void switch_to_other_solution(const std::vector<Cell> &cells, const std::unordered_set<int> &sequential_cells_id,const std::unordered_map<int,std::unordered_set<int>> &clk_group_id_to_ff_cell_ids,const std::unordered_map<int,int> &ff_cell_id_to_clk_group_id){
        set_cells(cells);
        reassign_pins_cell_id();
        set_sequential_cells_id(sequential_cells_id);
        set_clk_group_id_to_ff_cell_ids(clk_group_id_to_ff_cell_ids);
        set_ff_cell_id_to_clk_group_id(ff_cell_id_to_clk_group_id);        
    }
    void set_cells(const std::vector<Cell> &cells){
        this->cells = cells;
    }
    void set_sequential_cells_id(const std::unordered_set<int> &sequential_cells_id){
        this->sequential_cells_id = sequential_cells_id;
    }
    void set_pins(const std::vector<Pin> &pins){
        this->pins = pins;
    }
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
    bool check_cells_location();
    void reassign_pins_cell_id();
    int swap_ff(int cell_id,int new_lib_cell_id);
    int cluster_clk_group(const std::vector<std::vector<int>> &clustering_res);
    void update_cell(const Cell &cell){
        cells[cell.get_id()] = cell;
    }
private:
    // sequential cells
    std::vector<Pin> pins;
    std::vector<Cell> cells;
    std::unordered_set<int> sequential_cells_id;    
    // clk groups
    std::unordered_map<int,std::unordered_set<int>> clk_group_id_to_ff_cell_ids;
    std::unordered_map<int,int> ff_cell_id_to_clk_group_id;

private:
    Netlist() {} // Private constructor to prevent instantiation
    ~Netlist() {} // Private destructor to prevent deletion
    Netlist(const Netlist&) = delete; // Delete copy constructor
    Netlist& operator=(const Netlist&) = delete; // Delete assignment operator
};

} // namespace circuit

#endif // CIRCUIT_H