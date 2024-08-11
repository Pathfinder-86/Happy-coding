#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>
#include "cell.h"
#include <unordered_set>
namespace circuit {    
    class Solution{
        public:
            Solution(){}
            Solution(std::vector<Cell> cells, double cost):cells(cells),cost(cost){
                init_sequential_cells_id();
            }
            Solution(std::vector<Cell> cells):cells(cells){
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
            std::unordered_set<int> get_sequential_cells_id(){
                return sequential_cells_id;
            }
        private:
            std::vector<Cell> cells;
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
            void set_init_solution(const Solution &solution){
                init_solution = solution;
                current_solution = solution;
                best_solution = solution;
            }
            void set_current_solution(const Solution &solution){
                current_solution = solution;
            }
            void set_best_solution(const Solution &solution){
                best_solution = solution;
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
            double get_init_cost() const{
                return init_solution.get_cost();
            }
            double get_current_cost() const{
                return current_solution.get_cost();
            }
            double get_best_cost() const{
                return best_solution.get_cost();
            }
        private:
            Solution best_solution;
            Solution init_solution;
            Solution current_solution;
    };
}


#endif // SOLUTION_H