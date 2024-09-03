#ifndef COST_H
#define COST_H
#include <vector>
#include <iostream>
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
        CostCalculator():timing_cost(0.0),power_cost(0.0),area_cost(0.0){}
    public:
        static CostCalculator& get_instance(){
            static CostCalculator instance;
            return instance;
        }
        void calculate_cost();
        void update_cells_cost_after_clustering_skip_timer(const std::vector<int> &cells_id);
        void rollack_clustering_res(const std::vector<int> &cells_id);
        void rollack_timer_res(const std::vector<int> &cells_id);
        double get_cost() const {            
            return timing_cost + power_cost + area_cost + get_utilization_cost();
        }
        void print_cost() const {
            std::cout<<"Cost: "<<timing_cost<<" "<<power_cost<<" "<<area_cost<<" "<<get_utilization_cost()<<std::endl;
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
        double get_utilization_cost() const;
        void set_timing_cost(double timing_cost){
            this->timing_cost = timing_cost;
        }
        void set_power_cost(double power_cost){
            this->power_cost = power_cost;
        }
        void set_area_cost(double area_cost){
            this->area_cost = area_cost;
        }
        void add_timing_cost(double timing_cost){
            this->timing_cost += timing_cost;
        }
        void add_power_cost(double power_cost){
            this->power_cost += power_cost;
        }
        void add_area_cost(double area_cost){
            this->area_cost += area_cost;
        }                       
        void reset(){
            timing_cost = 0.0;
            power_cost = 0.0;
            area_cost = 0.0;
            sequential_cells_cost.clear();
        } 
        const std::vector<CellCost>& get_sequential_cells_cost() const {
            return sequential_cells_cost;
        }
        void update_cell_cost_by_libcell_id(int cell_id,int lib_cell_id);
        void update_cell_cost_by_slack(int cell_id,double slack);
        void update_cell_cost_been_clustered(int cell_id);
        void set_factors();
        void init(){
            set_factors();
            calculate_cost();
        }        
        void update_cells_cost_after_clustering(const std::vector<int> &clustering_cells_id,const std::vector<int> &timing_cells_id);
        const CellCost &get_cell_cost(int cell_id) const{
            return sequential_cells_cost[cell_id];
        }
        double get_cell_timing_cost(int cell_id) const{
            return sequential_cells_cost[cell_id].get_timing_cost();
        }        
    private:
        double timing_cost,power_cost,area_cost;
        std::vector<CellCost> sequential_cells_cost;
        double timing_factor,power_factor,area_factor,utilization_factor;
};
}
// namespace estimator

#endif // COST_H