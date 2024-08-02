#ifndef DESIGN_H
#define DESIGN_H
#include <vector>
#include <bin.h>
#include <libcell.h>
#include <unordered_map>
#include <stdexcept>
namespace design {

struct CostFactor {
    double timing_factor = 0.0;
    double power_factor = 0.0;
    double area_factor = 0.0;
    double utilization_factor = 0.0;
    double displacement_delay = 0.0;
};

struct Row {
    double x,y;
    double width,height;
    double site_width;    
};

class Design {
public:
    static Design& get_instance() {
        static Design instance;
        return instance;
    }
    // cost funtcion
    void set_timing_factor(double timing_factor) {
        cost_factor.timing_factor = timing_factor;
    }
    void set_power_factor(double power_factor) {
        cost_factor.power_factor = power_factor;
    }
    void set_area_factor(double area_factor) {
        cost_factor.area_factor = area_factor;
    }
    void set_utilization_factor(double utilization_factor) {
        cost_factor.utilization_factor = utilization_factor;
    }
    void set_displacement_delay(double displacement_delay) {
        cost_factor.displacement_delay = displacement_delay;
    }
    double get_timing_factor() const {
        return cost_factor.timing_factor;
    }
    double get_power_factor() const {
        return cost_factor.power_factor;
    }
    double get_area_factor() const {
        return cost_factor.area_factor;
    }
    double get_utilization_factor() const {
        return cost_factor.utilization_factor;
    }
    double get_displacement_delay() const {
        return cost_factor.displacement_delay;
    }
    // die boundary
    void add_die_boundary(double boundary) {
        die_boundaries.push_back(boundary);
    }
    const std::vector<double>& get_die_boundaries() const {
        return die_boundaries;
    }
    double get_die_width() const {
        return die_boundaries.at(2) - die_boundaries.at(0);
    }
    double get_die_height() const {
        return die_boundaries.at(3) - die_boundaries.at(1);
    }
    // bin
    void set_bin_size(double x, double y) {
        bin_size = std::make_pair(x, y);
    }
    double get_bin_width() const {
        return bin_size.first;
    }
    double get_bin_height() const {
        return bin_size.second;
    }
    void set_bin_max_utilization(double utilization) {
        bin_max_utilization = utilization;
    }
    double get_bin_max_utilization() const {
        return bin_max_utilization;
    }
    void init_bins();
    const std::vector<std::vector<Bin>>& get_bins() const {
        return bins;
    }
    void update_bins_utilization();
    // lib_cell
    void add_lib_cell(LibCell& lib_cell) {
        int id = lib_cells.size();
        lib_cell.set_id(id);
        lib_cells_id_map[lib_cell.get_name()] = id;        
        lib_cells.push_back(lib_cell);
    }
    const LibCell& get_lib_cell(int id) const {
        if (id >= 0 && id < static_cast<int>(lib_cells.size())) {
            return lib_cells.at(id);
        } else {
            throw std::out_of_range("Invalid lib cell ID");
        }        
    }
    const LibCell& get_lib_cell(const std::string& name) const {
        if (lib_cells_id_map.find(name) != lib_cells_id_map.end()) {
            return lib_cells.at(lib_cells_id_map.at(name));
        } else {
            throw std::out_of_range("Invalid lib cell name");
        }
    }
    void set_qpin_delay(const std::string& name,double delay){
        if (lib_cells_id_map.count(name)){
            int id = lib_cells_id_map.at(name);
            lib_cells.at(id).set_delay(delay);
        } else {
            throw std::out_of_range("Invalid lib cell name");
        }
    }
    void set_gate_power(const std::string& name,double power){
        if (lib_cells_id_map.count(name)){
            int id = lib_cells_id_map.at(name);
            lib_cells.at(id).set_power(power);
        } else {
            throw std::out_of_range("Invalid lib cell name");
        }
    }
    void add_flipflop_id_to_bits_group(int bits, int id){
        bits_flipflop_id_map[bits].push_back(id);
    }
    std::vector<int> get_bit_flipflops_id(int bit) const {
        if(!bits_flipflop_id_map.count(bit)){
            return {};
        }else {
            return bits_flipflop_id_map.at(bit);
        }
    }

    void add_row(double x, double y, double width, double height, double site_width){
        rows.push_back(Row{x,y,width,height,site_width});
    }
        
private:    
    CostFactor cost_factor;
    std::vector<double> die_boundaries;
    std::pair<double, double> bin_size;
    double bin_max_utilization;
    std::vector<std::vector<Bin>> bins;
    std::vector<LibCell> lib_cells;
    std::unordered_map<int,std::vector<int>> bits_flipflop_id_map;
    std::unordered_map<std::string, int> lib_cells_id_map;
    std::vector<Row> rows;
private:
    Design() {} // Private constructor to prevent instantiation
    Design(const Design&) = delete; // Delete copy constructor
    Design& operator=(const Design&) = delete; // Delete assignment operator
};
}

#endif // DESIGN_H