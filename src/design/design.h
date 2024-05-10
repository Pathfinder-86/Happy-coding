#ifndef DESIGN_H
#define DESIGN_H
#include <vector>
#include <bin.h>
namespace design {

struct CostFactor {
    double timing_factor = 0.0;
    double power_factor = 0.0;
    double area_factor = 0.0;
    double utilization_factor = 0.0;
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

private:
    CostFactor cost_factor;
    std::vector<double> die_boundaries;
    std::pair<double, double> bin_size;
    double bin_max_utilization;
    std::vector<std::vector<Bin>> bins;

private:
    Design() {} // Private constructor to prevent instantiation
    Design(const Design&) = delete; // Delete copy constructor
    Design& operator=(const Design&) = delete; // Delete assignment operator
};
}

#endif // DESIGN_H