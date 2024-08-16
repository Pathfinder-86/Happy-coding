#ifndef ESTIMATOR_LIB_CELL_EVALUATOR_H
#define ESTIMATOR_LIB_CELL_EVALUATOR_H
#include <vector>
#include <unordered_map>
namespace estimator {
    struct FFLibCellCost{
        private:
            int lib_cell_id;
            double timing_cost;
            double power_cost;
            double area_cost;
        public:
            FFLibCellCost(int lib_cell_id, double timing_cost, double power_cost, double area_cost):lib_cell_id(lib_cell_id),timing_cost(timing_cost),power_cost(power_cost),area_cost(area_cost){}
            int get_id() const {
                return lib_cell_id;
            }
            double get_timing_cost() const {
                return timing_cost;
            }
            double get_power_cost() const {
                return power_cost;
            }
            double get_area_cost() const {
                return area_cost;
            }
            double get_total_cost() const {
                return timing_cost + power_cost + area_cost;
            }
    };
    class FFLibcellCostManager {        
        public:
            void init(){
                calculate_cost();
                sort_by_cost();
            }
            void calculate_cost();
            void sort_by_cost();
        private:
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_total_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_power_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_area_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_timing_cost;

    };
}

#endif // ESTIMATOR_LIB_CELL_EVALUATOR_H