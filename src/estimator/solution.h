#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include "../circuit/cell.h"
#include <unordered_set>
namespace estimator {    
    class Solution{
        public:
            Solution(){}
            Solution(std::vector<circuit::Cell> cells, double cost):cells(cells),cost(cost){
                init_sequential_cells_id();
            }
            Solution(std::vector<circuit::Cell> cells):cells(cells){
                init_sequential_cells_id();
            }
            bool is_available() const{
                return cells.empty();
            }
            double get_cost() const{
                return cost;
            }
            void set_cost(double cost){
                this->cost = cost;
            }
            void init_sequential_cells_id(){
                sequential_cells_id.clear();
                for(auto &cell: cells){
                    if(cell.is_sequential() && cell.get_parent() == -1){
                        sequential_cells_id.insert(cell.get_id());
                    }
                }
            }
            bool is_sequential_cell(int cell_id){
                return sequential_cells_id.find(cell_id) != sequential_cells_id.end();
            }
            const std::unordered_set<int>& get_sequential_cells_id() const{
                return sequential_cells_id;
            }
            void update(const std::vector<circuit::Cell> &cells){
                this->cells = cells;
                init_sequential_cells_id();
            }
            bool is_available(){
                return !cells.empty();
            }
            const std::vector<circuit::Cell>& get_cells() const{
                return cells;
            }            
        private:
            std::vector<circuit::Cell> cells;
            std::unordered_set<int> sequential_cells_id;
            double cost = 0.0;
    };
    class SolutionManager{
        public:
            SolutionManager(){}
            static SolutionManager& get_instance(){
                static SolutionManager instance;
                return instance;
            }            
            Solution get_init_solution() const{
                return init_solution;
            }
            Solution get_current_solution() const{
                return current_solution;
            }
            Solution get_best_solution() const{
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
            void keep_init_solution(double cost = 0.0);
            void keep_best_solution();
            void keep_current_solution();

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
        private:
            Solution best_solution;
            Solution init_solution;
            Solution current_solution;
    };
}


#endif // SOLUTION_H