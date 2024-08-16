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
};

class CostCalculator{    
    private:
        CostCalculator():timing_cost(0.0),power_cost(0.0),area_cost(0.0),utilization_cost(0.0),total_cost(0.0){}
    public:
        static CostCalculator& get_instance(){
            static CostCalculator instance;
            return instance;
        }
        void calculate_cost();
        double get_cost() const {
            return total_cost;
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
            total_cost = 0.0;
        }        
    private:
        double timing_cost,power_cost,area_cost,utilization_cost,total_cost;
        std::vector<CellCost> sequential_cells_cost;
};
}
// namespace estimator

#endif // COST_H