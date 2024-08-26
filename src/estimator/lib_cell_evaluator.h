#ifndef ESTIMATOR_LIB_CELL_EVALUATOR_H
#define ESTIMATOR_LIB_CELL_EVALUATOR_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
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
            static FFLibcellCostManager& get_instance(){
                static FFLibcellCostManager instance;
                return instance;
            }
            FFLibcellCostManager(){}
            void init(){
                calculate_cost();
                sort_by_cost();
                find_best_libcell_bits();
                find_mid_bits_of_lib();
            }
            void calculate_cost();
            void sort_by_cost();
            const std::vector<FFLibCellCost>& get_bits_ff_libcells_sort_by_total_cost(int bits) const{
                if(bits_ff_libcells_sort_by_total_cost.find(bits) == bits_ff_libcells_sort_by_total_cost.end()){
                    return std::vector<FFLibCellCost>();
                }else{
                    return bits_ff_libcells_sort_by_total_cost.at(bits);
                }
            }
            const std::vector<FFLibCellCost>& get_bits_ff_libcells_sort_by_power_cost(int bits) const{
                if(bits_ff_libcells_sort_by_power_cost.find(bits) == bits_ff_libcells_sort_by_power_cost.end()){
                    return std::vector<FFLibCellCost>();
                }else{
                    return bits_ff_libcells_sort_by_power_cost.at(bits);
                }
            }
            const std::vector<FFLibCellCost>& get_bits_ff_libcells_sort_by_area_cost(int bits) const{
                if(bits_ff_libcells_sort_by_area_cost.find(bits) == bits_ff_libcells_sort_by_area_cost.end()){
                    return std::vector<FFLibCellCost>();
                }else{
                    return bits_ff_libcells_sort_by_area_cost.at(bits);
                }
            }
            const std::vector<FFLibCellCost>& get_bits_ff_libcells_sort_by_timing_cost(int bits) const{
                if(bits_ff_libcells_sort_by_timing_cost.find(bits) == bits_ff_libcells_sort_by_timing_cost.end()){
                    return std::vector<FFLibCellCost>();
                }else{
                    return bits_ff_libcells_sort_by_timing_cost.at(bits);
                }
            }
            int get_best_total_cost_lib_cell_id(int bits) const{
                if(bits_ff_libcells_sort_by_total_cost.find(bits) == bits_ff_libcells_sort_by_total_cost.end()){
                    return -1;
                }else{
                    return bits_ff_libcells_sort_by_total_cost.at(bits).front().get_id();
                }
            }
            int get_best_power_cost_lib_cell_id(int bits) const{
                if(bits_ff_libcells_sort_by_power_cost.find(bits) == bits_ff_libcells_sort_by_power_cost.end()){
                    return -1;
                }else{
                    return bits_ff_libcells_sort_by_power_cost.at(bits).front().get_id();
                }
            }
            int get_best_area_cost_lib_cell_id(int bits) const{
                if(bits_ff_libcells_sort_by_area_cost.find(bits) == bits_ff_libcells_sort_by_area_cost.end()){
                    return -1;
                }else{
                    return bits_ff_libcells_sort_by_area_cost.at(bits).front().get_id();
                }
            }
            int get_best_timing_cost_lib_cell_id(int bits) const{
                if(bits_ff_libcells_sort_by_timing_cost.find(bits) == bits_ff_libcells_sort_by_timing_cost.end()){
                    return -1;
                }else{
                    return bits_ff_libcells_sort_by_timing_cost.at(bits).front().get_id();
                }
            }
            double get_lib_cell_cost(int lib_cell_id) const{
                if(ff_libcells_cost.find(lib_cell_id) == ff_libcells_cost.end()){
                    return 0.0;
                }else{
                    return ff_libcells_cost.at(lib_cell_id).get_total_cost();
                }
            }
            int find_mid_bits_of_lib();
            int get_mid_bits_of_lib() const{
                return mid_bits_of_lib;
            }

            void find_best_libcell_bits();
            const std::unordered_set<int>& get_bits_num() const{
                return bits_num;
            }
            bool is_bits_libcell_exsit(int bits) const{
                return bits_num.find(bits) != bits_num.end();
            }
            int get_best_libcell_for_bit(int bits) const{
                if(best_libcell_bits.find(bits) == best_libcell_bits.end()){
                    return -1;
                }else{
                    return best_libcell_bits.at(bits);
                }
            }
            const std::vector<int>& get_best_libcell_sorted_by_bits() const{
                return best_libcell_sorted_by_bits;
            }
        private:
            std::unordered_map<int,FFLibCellCost> ff_libcells_cost; // lib_cell_id -> cost            
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_total_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_power_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_area_cost;
            std::unordered_map<int,std::vector<FFLibCellCost>> bits_ff_libcells_sort_by_timing_cost;
            int mid_bits_of_lib;
            std::unordered_map<int,int> best_libcell_bits;
            std::unordered_set<int> bits_num;
            std::vector<int> best_libcell_sorted_by_bits;
    };
}

#endif // ESTIMATOR_LIB_CELL_EVALUATOR_H