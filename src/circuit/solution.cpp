#include "netlist.h"
#include "cell.h"
#include "solution.h"
namespace circuit {    
void SolutionManager::keep_best_solution(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    best_solution.update(netlist.get_cells());
}

void SolutionManager::keep_current_solution(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    current_solution.update(netlist.get_cells());
}

void SolutionManager::keep_init_solution(double cost){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    init_solution.update(netlist.get_cells());
    init_solution.set_cost(cost);
    current_solution = init_solution;
    best_solution = init_solution;
}
}