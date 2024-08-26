#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include "../circuit/cell.h"
#include "../timer/timer.h"
#include "../legalizer/legalizer.h"
#include "cost.h"
#include <unordered_set>
#include "../circuit/netlist.h"
#include <unordered_map>
namespace estimator {    
    class Solution{
        public:
            Solution():cost(0.0){}
            Solution(double cost,const std::vector<circuit::Cell> &cells,const std::unordered_map<int,timer::TimingNode> &timing_nodes,const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map):cost(cost),cells(cells),timing_nodes(timing_nodes),cell_id_to_site_id_map(cell_id_to_site_id_map){}
            bool is_available() const{
                return !cells.empty();
            }       
            void update_cost(double cost){
                this->cost = cost;
            }
            void update_netlist(const std::vector<circuit::Cell> &cells,const std::unordered_set<int> &sequential_cells_id){
                this->cells = cells;
                this->sequential_cells_id = sequential_cells_id;                
            }
            void update_netlist(const circuit::Netlist &netlist){
                this->cells = netlist.get_cells();
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
            void update(double cost,const std::vector<circuit::Cell> &cells, const std::unordered_set<int> sequentail_cells_id,const std::unordered_map<int,timer::TimingNode> &timing_nodes, const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map){
                this->cost = cost;
                this->cells = cells;
                this->sequential_cells_id = sequentail_cells_id;
                this->timing_nodes = timing_nodes;
                this->cell_id_to_site_id_map = cell_id_to_site_id_map;                
            }
            void update(const CostCalculator &cost_calculator, const circuit::Netlist &netlist, const timer::Timer &timer, const legalizer::Legalizer &legalizer){
                this->cost = cost_calculator.get_cost();
                this->cells = netlist.get_cells();
                this->sequential_cells_id = netlist.get_sequential_cells_id();
                this->timing_nodes = timer.get_timing_nodes();
                this->cell_id_to_site_id_map = legalizer.get_cell_id_to_site_id_map();
            }
            double get_cost() const{
                return cost;
            }   
            const std::vector<circuit::Cell>& get_cells() const{
                return cells;
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
            void update_cells_by_id(const std::vector<int> &cells_id);
            void rollback_cells_by_id(const std::vector<int> &cells_id);
            void remove_cell_from_clk_group(int cell_id){
                if(ff_cell_id_to_clk_group_id.count(cell_id)){
                    int clk_group_id = ff_cell_id_to_clk_group_id[cell_id];
                    ff_cell_id_to_clk_group_id.erase(cell_id);
                    clk_group_id_to_ff_cell_ids[clk_group_id].erase(cell_id);
                }
            }
            void remove_sequential_cell(int cell_id){
                sequential_cells_id.erase(cell_id);
            }
            void remove_cell_id_to_sites_id(int cell_id){
                cell_id_to_site_id_map.erase(cell_id);
            }
            void rollback_netlist(const std::vector<int> &cells_id);
            void add_sequential_cell(int cell_id){
                sequential_cells_id.insert(cell_id);
            }
            void add_cell_to_clk_group(int cell_id,int clk_group_id){
                ff_cell_id_to_clk_group_id[cell_id] = clk_group_id;
                clk_group_id_to_ff_cell_ids[clk_group_id].insert(cell_id);
            }
            void add_cell_id_to_site_id(int cell_id,const std::vector<int> &site_ids){
                cell_id_to_site_id_map[cell_id] = site_ids;
            }
            void update_single_cell(const circuit::Cell &cell){
                cells[cell.get_id()] = cell;
            }
            const circuit::Cell &get_cell(int cell_id) const{
                return cells[cell_id];
            }
            int get_clk_group_id(int cell_id) const{
                if(ff_cell_id_to_clk_group_id.count(cell_id)){
                    return ff_cell_id_to_clk_group_id.at(cell_id);
                }else{
                    return -1;
                }
            }
            const std::vector<int> & get_cell_site_ids(int cell_id) const{
                return cell_id_to_site_id_map.at(cell_id);
            }

        private:
            double cost;
            // netlist
            std::vector<circuit::Cell> cells;   
            std::unordered_set<int> sequential_cells_id;
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
                keep_solution(1);
                keep_solution(2);
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
            void update_best_solution_by_cells_id(const std::vector<int> &cells_id,double cost);
            void rollack_best_solution_by_cells_id(const std::vector<int> &cells_id);            
        private:
            Solution best_solution;
            Solution init_solution;
            Solution current_solution;
    };
}


#endif // SOLUTION_H