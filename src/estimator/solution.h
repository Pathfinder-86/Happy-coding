#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include "../circuit/cell.h"
#include "../timer/timer.h"
#include "../circuit/pin.h"
#include "../legalizer/legalizer.h"
#include "cost.h"
#include <unordered_set>
#include <unordered_map>
namespace estimator {    
    class Solution{
        public:
            Solution():cost(0.0){}
            Solution(double cost,const std::vector<circuit::Cell> &ff_cells,const std::vector<circuit::Pin> &ff_pins,const std::unordered_map<int,timer::TimingNode> &timing_nodes,const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map):cost(cost),ff_cells(ff_cells),ff_pins(ff_pins),timing_nodes(timing_nodes),cell_id_to_site_id_map(cell_id_to_site_id_map){}
            bool is_available() const{
                return !ff_cells.empty();
            }       
            void update_cost(double cost){
                this->cost = cost;
            }
            void update_netlist(const std::vector<circuit::Cell> &ff_cells,const std::vector<circuit::Pin> &ff_pins,const std::unordered_set<int> &sequential_cells_id){
                this->ff_cells = ff_cells;
                this->ff_pins = ff_pins;
                this->sequential_cells_id = sequential_cells_id;                
            }
            void update_netlist(const circuit::Netlist &netlist){
                this->ff_cells = netlist.get_ff_cells();
                this->ff_pins = netlist.get_ff_pins();
                this->sequential_cells_id = netlist.get_sequential_cells_id();
                this->clk_group_id_to_ff_cell_ids = netlist.get_clk_group_id_to_ff_cell_ids();
                this->ff_cell_id_to_clk_group_id = netlist.get_ff_cell_id_to_clk_group_id();
            }
            void update_timer(const std::unordered_map<int,timer::TimingNode> &timing_nodes){
                this->timing_nodes = timing_nodes;
            }
            void update_timer(const timer::Timer &timer){
                this->timing_nodes = timer.get_timing_nodes();
            }
            void update_legalizer(const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map){
                this->cell_id_to_site_id_map = cell_id_to_site_id_map;
            }
            void update_legalizer(const legalizer::Legalizer &legalizer){
                this->cell_id_to_site_id_map = legalizer.get_cell_id_to_site_id_map();
            }
            void update(double cost,const std::vector<circuit::Cell> &ff_cells, const std::vector<circuit::Pin> &ff_pins,const std::unordered_set<int> sequentail_cells_id,const std::unordered_map<int,timer::TimingNode> &timing_nodes, const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map){
                this->cost = cost;
                this->ff_cells = ff_cells;
                this->ff_pins = ff_pins;
                this->sequential_cells_id = sequentail_cells_id;
                this->timing_nodes = timing_nodes;
                this->cell_id_to_site_id_map = cell_id_to_site_id_map;                
            }
            void update(const CostCalculator &cost_calculator, const circuit::Netlist &netlist, const timer::Timer &timer, const legalizer::Legalizer &legalizer){
                this->cost = cost_calculator.get_cost();
                this->ff_cells = netlist.get_ff_cells();
                this->ff_pins = netlist.get_ff_pins();
                this->sequential_cells_id = netlist.get_sequential_cells_id();
                this->timing_nodes = timer.get_timing_nodes();
                this->cell_id_to_site_id_map = legalizer.get_cell_id_to_site_id_map();
            }
            double get_cost() const{
                return cost;
            }   
            const std::vector<circuit::Cell>& get_ff_cells() const{
                return ff_cells;
            }
            const std::vector<circuit::Pin>& get_ff_pins() const{
                return ff_pins;
            }
            const std::unordered_set<int>& get_sequential_cells_id() const{
                return sequential_cells_id;
            }
            const std::unordered_map<int,timer::TimingNode>& get_timing_nodes() const{
                return timing_nodes;
            }
            const std::unordered_map<int,std::vector<int>>& get_cell_id_to_site_id_map() const{
                return cell_id_to_site_id_map;
            } 
            const std::unordered_map<int,std::unordered_set<int>>& get_clk_group_id_to_ff_cell_ids() const{
                return clk_group_id_to_ff_cell_ids;
            } 
            const std::unordered_map<int,int>& get_ff_cell_id_to_clk_group_id() const{
                return ff_cell_id_to_clk_group_id;
            }
        private:
            double cost;
            // netlist
            std::vector<circuit::Cell> ff_cells;
            std::vector<circuit::Pin> ff_pins;            
            // clk groups
            std::unordered_map<int,std::unordered_set<int>> clk_group_id_to_ff_cell_ids;
            std::unordered_map<int,int> ff_cell_id_to_clk_group_id;            
            // timer
            std::unordered_map<int,timer::TimingNode> timing_nodes;  
            // legalizer
            std::unordered_map<int,std::vector<int>> cell_id_to_site_id_map;

    };
    class SolutionManager{
        public:
            SolutionManager(){}
            static SolutionManager& get_instance(){
                static SolutionManager instance;
                return instance;
            }            
            const Solution &get_init_solution() const{
                return init_solution;
            }
            const Solution &get_current_solution() const{
                return current_solution;
            }
            const Solution &get_best_solution() const{
                return best_solution;
            }
            Solution &get_mutable_current_solution(){
                return current_solution;
            }
            Solution &get_mutable_best_solution(){
                return best_solution;
            }            
            double get_init_cost() const{
                return init_solution.get_cost();
            }
            double get_current_cost() const{
                return current_solution.get_cost();
            }
            double get_best_cost() const{
                return best_solution.get_cost();
            }
            void keep_solution(int which_solution);
            void keep_init_solution(){
                keep_solution(0);
                keep_current_solution();
                keep_best_solution();
            };
            void keep_best_solution(){
                keep_solution(2);
            }
            void keep_current_solution(){
                keep_solution(1);
            }
            void print_cost(const Solution&) const;
            void print_best_solution() const{
                print_cost(best_solution);
            }
            void print_current_solution() const{
                print_cost(current_solution);
            }
            void print_init_solution() const{
                print_cost(init_solution);
            }
            void switch_to_other_solution(const Solution &solution);
            void switch_to_best_solution(){
                switch_to_other_solution(best_solution);
            }
            void switch_to_current_solution(){
                switch_to_other_solution(current_solution);
            }            
        private:
            Solution best_solution;
            Solution init_solution;
            Solution current_solution;
    };
}


#endif // SOLUTION_H