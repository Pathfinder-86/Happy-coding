#ifndef COST_H
#define COST_H

namespace estimator {

class CostCalculator{
    public:
        CostCalculator():timing_cost(0.0),power_cost(0.0),area_cost(0.0),utilization_cost(0.0),total_cost(0.0){}
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
};
}
// namespace estimator

#endif // COST_H