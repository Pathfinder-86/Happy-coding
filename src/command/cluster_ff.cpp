#include "../circuit/netlist.h"
#include <unordered_set>
#include "command_manager.h"
#include "../estimator/solution.h"
namespace command{

void CommandManager::test_cluster_ff() {    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();    
    // test
    std::cout<<"COMMAND:: test cluster_ff"<<std::endl;
    std::unordered_set<int> ff_cells_id;
    ff_cells_id = netlist.get_sequential_cells_id();
    if(ff_cells_id.size() < 2){
        std::cout<<"No enough FF cells to cluster"<<std::endl;
        return;
    }
    // cluster first two FF cells
    int first_cell_id = *ff_cells_id.begin();
    int second_cell_id = *(++ff_cells_id.begin());
    netlist.cluster_cells(first_cell_id,second_cell_id);
    std::cout<<"test_cluster_ff::Clustered two FF cells"<<std::endl;
    // update solution
    estimator::SolutionManager &solution_manager = estimator::SolutionManager::get_instance();
    solution_manager.keep_current_solution();
    solution_manager.keep_best_solution();
}
}