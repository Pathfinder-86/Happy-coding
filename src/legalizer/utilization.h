#ifndef UTILIZATION_H
#define UTILIZATION_H
#include "../design/bin.h"
#include <vector>
#include <set>
namespace legalizer {
class UtilizationCalculator {
    public:        
        UtilizationCalculator(){};
        static UtilizationCalculator& get_instance() {
            static UtilizationCalculator instance;
            return instance;
        }
        void init_utilization_calculator();
        void update_bins_utilization();
        void reset_bins_utilization();
        int get_overflow_bins_num() const{
            return overflow_bins_id.size();
        }
        void update_overflow_bins();
        double add_cell_cost_change(int cell_id);
        void add_cell(int cell_id);
        void remove_cell(int cell_id);
        const std::vector<std::vector<design::Bin>>& get_bins() const {
            return bins;
        }
        const std::set<std::pair<int, int>>& get_overflow_bins_id() const {
            return overflow_bins_id;
        }
        void remove_cells(const std::vector<int> &cells_id);
        void add_cells(const std::vector<int> &cells_id);
        bool is_overflow_bin(int i, int j){
            return bins[i][j].get_utilization() >= max_utilization;
        }
        bool is_overflow_utilization(double utilization) const{
            return utilization >= max_utilization;
        }
        double get_bin_area() const{
            return bin_area;
        }
        int get_bin_width_int() const{
            return bin_width_int;
        }
        int get_bin_height_int() const{
            return bin_height_int;
        }
        double remove_cells_cost_change(const std::vector<int> &cells_id);
        void print();
    private:
        std::vector<std::vector<design::Bin>> bins;
        std::vector<std::vector<design::Bin>> init_bins;
        std::set<std::pair<int, int>> overflow_bins_id;
        int init_overflow_bins;
        int bin_width_int, bin_height_int;
        double bin_area;
        double max_utilization;

};
}
#endif // UTILIZATION_H