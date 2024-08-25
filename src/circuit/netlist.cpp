#include "netlist.h"
#include <iostream>
#include <../timer/timer.h>
#include <../estimator/lib_cell_evaluator.h>
#include <../legalizer/legalizer.h>
#include "original_netlist.h"
namespace circuit {

bool Netlist::check_cells_location(){
    OriginalNetlist &original_netlist = OriginalNetlist::get_instance();
    std::vector<Cell> removed_clustered_cells;
    for(const Cell &cell : cells){
        if(!cell.is_clustered()){
            removed_clustered_cells.push_back(cell);
        }
    }
    bool cell_location_legal =  original_netlist.check_cells_location(removed_clustered_cells);
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();    
    bool legal = legalizer.check_on_site();
    if(!cell_location_legal){
        std::cout<<"ERROR! cells overlap or cell out of die\n";        
    }
    if(!legal){
        std::cout<<"ERROR! cells are not legal(on-site)\n";
    }
    return cell_location_legal && legal;
}

// return 0: success , return 1: legalize fail, return 2: others
int Netlist::cluster_cells(const std::vector<int> &cells_id){    
    // duplicate cell_id in cells_id
    std::unordered_set<int> cells_id_set(cells_id.begin(), cells_id.end());
    if(cells_id_set.size() != cells_id.size()){
        std::cout<<"CLSUTER:: Cannot cluster cells that have duplicate cell_id\n";
        return 3;
    }
    // cluster less than 2 cells
    if(cells_id.size() < 2){
        std::cout<<"CLSUTER:: Cannot cluster less than 2 cells\n";
        return 3;
    }
    // is clustered
    for(int cell_id : cells_id){
        const Cell &cell = get_cell(cell_id);
        if(cell.is_clustered()){
            std::cout<<"CLSUTER:: Cannot cluster cells that are sequential cells\n";
            return 3;
        }
    }
    // must in same clk net
    int clk_group = -1;
    for(int cell_id : cells_id){
        int cell_clk_group_id = get_clk_group_id(cell_id);
        if(cell_clk_group_id == -1){
            std::cout<<"CLSUTER:: Cannot cluster cells that are not in the clock net\n";
            return 3;
        }
        
        if(clk_group == -1){
            clk_group = cell_clk_group_id;
        }else{
            if(clk_group != cell_clk_group_id){
                std::cout<<"CLSUTER:: Cannot cluster cells that are not in the same clock group\n";
                return 3;
            }
        }
    }

    // get new lib_cell
    int new_bits = 0;
    for(int cell_id : cells_id){
        const Cell &cell = get_cell(cell_id);
        new_bits += cell.get_bits();
    }

    // Get best cost flip-flop
    const estimator::FFLibcellCostManager &ff_libcell_cost_manager = estimator::FFLibcellCostManager::get_instance();
    int best_lib_cell_id = ff_libcell_cost_manager.get_best_total_cost_lib_cell_id(new_bits);
    
    if(best_lib_cell_id == -1){
        std::cout<<"CLSUTER:: Cannot find a flip-flop with " << new_bits << " bits\n";
        return 2;
        // need to add clusters num to downsize the flip-flop
    }
    return try_legal_and_modify_circuit_since_merge_cell(cells_id, best_lib_cell_id);
}

// return 0: success , return 1: legalize fail, return 2: no ff, return 3: others
int Netlist::try_legal_and_modify_circuit_since_merge_cell(const std::vector<int> &cells_id, const int new_lib_cell_id){
    // parent
    int parent_id = cells_id.at(0);
    Cell& cell1 = get_mutable_cell(parent_id);
    // new cell location
    // TODO: aummption: center of cells_id    
    int new_cell_x = 0,new_cell_y = 0;
    for(int cell_id : cells_id){
        const Cell &cell = get_cell(cell_id);
        new_cell_x += cell.get_x();
        new_cell_y += cell.get_y();
    }
    new_cell_x /= cells_id.size();
    new_cell_y /= cells_id.size();
    cell1.set_x(new_cell_x);
    cell1.set_y(new_cell_y);

    // set cell1 to new_lib_cell
    cell1.set_lib_cell_id(new_lib_cell_id);
    const design::Design &design = design::Design::get_instance(); 
    const design::LibCell &new_lib_cell = design.get_lib_cell(new_lib_cell_id);    
    cell1.set_w(new_lib_cell.get_width());
    cell1.set_h(new_lib_cell.get_height());
    
    
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    legalizer.replacement_cell(parent_id);
    for(int i=1;i<static_cast<int>(cells_id.size());i++){        
        legalizer.remove_cell(cells_id[i]);        
    }
    if(!legalizer.legalize()){
        std::cout<<"CLSUTER:: legalization fail\n";
        return 1;
        // need to find legalization fail reason and adjust circuit to cluster more ff
    }


    // handle pins mapping    
    std::vector<int> new_cell_input_pins_id;
    std::vector<int> new_cell_output_pins_id;

    for(int i = 0; i < static_cast<int>(cells_id.size()); i++){
        const Cell &cell = get_cell(cells_id.at(i));
        std::vector<int> cell_input_pins_id = cell.get_input_pins_id();
        std::vector<int> cell_output_pins_id = cell.get_output_pins_id();        
        new_cell_input_pins_id.insert(new_cell_input_pins_id.end(), cell_input_pins_id.begin(), cell_input_pins_id.end());   
        new_cell_output_pins_id.insert(new_cell_output_pins_id.end(), cell_output_pins_id.begin(), cell_output_pins_id.end());        
    }
    cell1.set_input_pins_id(new_cell_input_pins_id);
    cell1.set_output_pins_id(new_cell_output_pins_id);

    // assign new location and owner
    std::vector<std::pair<double, double>> new_lib_cell_input_pins_position = new_lib_cell.get_input_pins_position();
    std::vector<std::pair<double, double>> new_lib_cell_output_pins_position = new_lib_cell.get_output_pins_position();    
    if(new_lib_cell_input_pins_position.size() != new_cell_input_pins_id.size()){
        std::cerr << "CLSUTER:: new_lib_cell_input_pins_position.size() != new_cell_input_pins_id.size()\n";
        return 3;
    }
    if(new_lib_cell_output_pins_position.size() != new_cell_output_pins_id.size()){
        std::cerr << "CLSUTER:: new_lib_cell_input_pins_position.size() != new_cell_input_pins_id.size()\n";
        return 3;
    }

    int bits = new_lib_cell_input_pins_position.size();
    for(int i = 0; i < bits; i++){
        int input_pin_id = new_cell_input_pins_id.at(i);
        int output_pin_id = new_cell_output_pins_id.at(i);
        Pin &input_pin = get_mutable_pin(input_pin_id);
        Pin &output_pin = get_mutable_pin(output_pin_id);
        input_pin.set_offset_x(new_lib_cell_input_pins_position.at(i).first);
        input_pin.set_offset_y(new_lib_cell_input_pins_position.at(i).second);
        input_pin.set_x(new_cell_x + new_lib_cell_input_pins_position.at(i).first);
        input_pin.set_y(new_cell_y + new_lib_cell_input_pins_position.at(i).second);
        input_pin.set_cell_id(parent_id); 

        output_pin.set_offset_x(new_lib_cell_output_pins_position.at(i).first);
        output_pin.set_offset_y(new_lib_cell_output_pins_position.at(i).second);
        output_pin.set_x(new_cell_x + new_lib_cell_output_pins_position.at(i).first);
        output_pin.set_y(new_cell_y + new_lib_cell_output_pins_position.at(i).second);
        output_pin.set_cell_id(parent_id);
    }        
    
    for(int i = 1; i < static_cast<int>(cells_id.size()); i++){
        int cell_id = cells_id.at(i);
        Cell &cell = get_mutable_cell(cell_id);
        cell.set_parent(parent_id);
        cell.clear();
        remove_sequential_cell(cell_id);
        remove_cell_from_clk_group(cell_id);        
    }

    // since other pins are already merged into parent_id, we don't need to update timing for clustered cells
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing(parent_id);
    return 0;
}

// return 0: success , return 1: legalize fail, return 2: no ff target to decluster,return 3: no ff for decluster, return 4: others
int Netlist::decluster_cells(int id){
    const design::Design &design = design::Design::get_instance();    
    const Cell& cell = get_cell(id);
    if(cell.is_clustered()){
        std::cout<<"DECLSUTER:: Cannot cluster a cell that is already clustered" << id<< std::endl;
        return 4;
    }
    
    int lib_cell_id = cell.get_lib_cell_id();    
    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
    int origin_bits = lib_cell.get_bits();
    if(origin_bits == 1){
        std::cout<<"DECLSUTER:: Cannot decluster a cell that is already a single flip-flop" << id<< std::endl;
        return 2;
    }
    // Get best cost flip-flop
    const estimator::FFLibcellCostManager &ff_libcell_cost_manager = estimator::FFLibcellCostManager::get_instance();
    // half the bits
    int new_bits1 = origin_bits / 2;
    int new_bits2 = origin_bits - new_bits1;

    int best_lib_cell_id1 = ff_libcell_cost_manager.get_best_total_cost_lib_cell_id(new_bits1);
    int best_lib_cell_id2 = ff_libcell_cost_manager.get_best_total_cost_lib_cell_id(new_bits2);
    
    if(best_lib_cell_id1 == -1){
        std::cout<<"DECLSUTER:: Cannot find a flip-flop with " << new_bits1 << " bits\n";
        return 3;
    }
    if(best_lib_cell_id2 == -1){
        std::cout<<"DECLSUTER:: Cannot find a flip-flop with " << new_bits2 << " bits\n";
        return 3;
    }

    return modify_circuit_since_divide_cell(id, best_lib_cell_id1, best_lib_cell_id2);    
}

int Netlist::modify_circuit_since_divide_cell(int cell_id, const int new_lib_cell_id1, const int new_lib_cell_id2){    
    // add new cell to cells    
    Cell temp;
    add_cell(temp);    
    int new_cell_id = temp.get_id();    
    Cell &new_cell = get_mutable_cell(new_cell_id);

    const design::Design &design = design::Design::get_instance();
    // original cell become cell1
    Cell& cell1 = get_mutable_cell(cell_id);
    cell1.set_lib_cell_id(new_lib_cell_id1);
    new_cell.set_lib_cell_id(new_lib_cell_id2);
    int new_cell_x = cell1.get_x();
    int new_cell_y = cell1.get_y();
    new_cell.set_x(new_cell_x);
    new_cell.set_y(new_cell_y);

    
    // change cell1 and new_cell width and height
    const design::LibCell &new_lib_cell1 = design.get_lib_cell(new_lib_cell_id1);
    const design::LibCell &new_lib_cell2 = design.get_lib_cell(new_lib_cell_id2);
    cell1.set_w(new_lib_cell1.get_width());
    cell1.set_h(new_lib_cell1.get_height());
    new_cell.set_w(new_lib_cell2.get_width());
    new_cell.set_h(new_lib_cell2.get_height()); 


    // handle pins mapping
    // give first half of the pins to cell1 and the rest to new_cell    
    int bits1 = new_lib_cell1.get_bits();
    int bits2 = new_lib_cell2.get_bits();
    int origin_bits = bits1 + bits2;

    // INPUT PINS and OUTPUT PINS    
    const std::vector<std::pair<double, double>> &new_lib_cell1_input_pins_position = new_lib_cell1.get_input_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell2_input_pins_position = new_lib_cell2.get_input_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell1_output_pins_position = new_lib_cell1.get_output_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell2_output_pins_position = new_lib_cell2.get_output_pins_position();    
    std::vector<int> origin_cell_input_pins_id = cell1.get_input_pins_id();
    std::vector<int> origin_cell_output_pins_id = cell1.get_output_pins_id();
    std::vector<int> new_cell_output_pins_id1, new_cell_output_pins_id2;
    std::vector<int> new_cell_input_pins_id1, new_cell_input_pins_id2;
    new_cell_input_pins_id1.reserve(bits1);
    new_cell_input_pins_id2.reserve(bits2);
    new_cell_output_pins_id1.reserve(bits1);
    new_cell_output_pins_id2.reserve(bits2);
    new_cell_input_pins_id1.insert(new_cell_input_pins_id1.end(), origin_cell_input_pins_id.begin(), origin_cell_input_pins_id.begin() + bits1);
    new_cell_input_pins_id2.insert(new_cell_input_pins_id2.end(), origin_cell_input_pins_id.begin() + bits1, origin_cell_input_pins_id.end());
    new_cell_output_pins_id1.insert(new_cell_output_pins_id1.end(), origin_cell_output_pins_id.begin(), origin_cell_output_pins_id.begin() + bits1);
    new_cell_output_pins_id2.insert(new_cell_output_pins_id2.end(), origin_cell_output_pins_id.begin() + bits1, origin_cell_output_pins_id.end());

    for(int i = 0; i < origin_bits; i++){
        int input_pin_id = origin_cell_input_pins_id.at(i);
        int output_pin_id = origin_cell_output_pins_id.at(i);
        Pin &input_pin = get_mutable_pin(input_pin_id);
        Pin &output_pin = get_mutable_pin(output_pin_id);
        int new_input_pin_x_offset,new_input_pin_y_offset;
        int new_output_pin_x_offset,new_output_pin_y_offset;
        if(i < bits1){
            // origin
            new_input_pin_x_offset = new_lib_cell1_input_pins_position.at(i).first;
            new_input_pin_y_offset = new_lib_cell1_input_pins_position.at(i).second;
            new_output_pin_x_offset = new_lib_cell1_output_pins_position.at(i).first;
            new_output_pin_y_offset = new_lib_cell1_output_pins_position.at(i).second;
        }else{
            // new
            new_input_pin_x_offset = new_lib_cell2_input_pins_position.at(i - bits1).first;
            new_input_pin_y_offset = new_lib_cell2_input_pins_position.at(i - bits1).second;
            new_output_pin_x_offset = new_lib_cell2_output_pins_position.at(i - bits1).first;
            new_output_pin_y_offset = new_lib_cell2_output_pins_position.at(i - bits1).second;
            input_pin.set_cell_id(new_cell_id);
            output_pin.set_cell_id(new_cell_id);
        }
        input_pin.set_offset_x(new_input_pin_x_offset);
        input_pin.set_x(new_cell_x + new_input_pin_x_offset);
        input_pin.set_offset_y(new_input_pin_y_offset);
        input_pin.set_y(new_cell_y + new_input_pin_y_offset);
        output_pin.set_offset_x(new_output_pin_x_offset);
        output_pin.set_x(new_cell_x + new_output_pin_x_offset);
        output_pin.set_offset_y(new_output_pin_y_offset);
        output_pin.set_y(new_cell_y + new_output_pin_y_offset);        
    }
    cell1.set_input_pins_id(new_cell_input_pins_id1);
    cells.at(new_cell_id).set_input_pins_id(new_cell_input_pins_id2);
    cell1.set_output_pins_id(new_cell_output_pins_id1);
    cells.at(new_cell_id).set_output_pins_id(new_cell_output_pins_id2);

   
    add_sequential_cell(new_cell_id);
    int clk_group_id = get_clk_group_id(cell_id);
    add_cell_to_clk_group(new_cell_id, clk_group_id);
    
    //std::cout<<"DECLUSTER:: legalizer"<<std::endl;
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();     
    legalizer.replacement_cell(cell_id);
    legalizer.add_cell(new_cell_id);
    if(!legalizer.legalize()){
        std::cout<<"DECLUSTER:: legalization fail\n";
        return 1;
    }

    // update timing information
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing(cell_id);
    timer.update_timing(new_cell_id);
    return 0;
}

void Netlist::reassign_pins_cell_id(){
    for(int cid : sequential_cells_id){
        const Cell &cell = get_cell(cid);
        int cell_id = cell.get_id();
        std::vector<int> input_pins_id = cell.get_input_pins_id();
        std::vector<int> output_pins_id = cell.get_output_pins_id();
        for(int pin_id : input_pins_id){
            Pin &pin = get_mutable_pin(pin_id);
            pin.set_cell_id(cell_id);
        }
        for(int pin_id : output_pins_id){
            Pin &pin = get_mutable_pin(pin_id);
            pin.set_cell_id(cell_id);
        }
    }
}

} // namespace circuit