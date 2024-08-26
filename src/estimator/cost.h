#ifndef COST_H
#define COST_H
#include <vector>

namespace estimator {

struct CellCost {
    private:
        int cell_id;
        double timing_cost;
        double power_cost;
        double area_cost;
    public:
        CellCost(int cell_id, double timing_cost, double power_cost, double area_cost):cell_id(cell_id),timing_cost(timing_cost),power_cost(power_cost),area_cost(area_cost){}
        int get_id() const {
            return cell_id;
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
        void set_zero(){
            timing_cost = 0.0;
            power_cost = 0.0;
            area_cost = 0.0;
        }
        void set_timing_cost(double timing_cost){
            this->timing_cost = timing_cost;
        }
        void set_power_cost(double power_cost){
            this->power_cost = power_cost;
        }
        void set_area_cost(double area_cost){
            this->area_cost = area_cost;
        }
};

class CostCalculator{    
    private:
        CostCalculator():timing_cost(0.0),power_cost(0.0),area_cost(0.0),utilization_cost(0.0){}
    public:
        static CostCalculator& get_instance(){
            static CostCalculator instance;
            return instance;
        }
        void calculate_cost();
        void calculate_cost_update_by_cells_id(const std::vector<int> &cells_id);
        void calculate_cost_rollback_by_cells_id(const std::vector<int> &cells_id);
        double get_cost() const {
            return timing_cost + power_cost + area_cost + utilization_cost;
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
        double get_utilization_cost() const {
            return utilization_cost;
        }
        void reset(){
            timing_cost = 0.0;
            power_cost = 0.0;
            area_cost = 0.0;
            utilization_cost = 0.0;
            sequential_cells_cost.clear();
        } 
        const std::vector<CellCost>& get_sequential_cells_cost() const {
            return sequential_cells_cost;
        }
        // sorted ?
    private:
        double timing_cost,power_cost,area_cost,utilization_cost;
        std::vector<CellCost> sequential_cells_cost;
};
}
// namespace estimator

#endif // COST_H