#include "netlist.h"
#include <iostream>
#include <../timer/timer.h>
#include <../estimator/lib_cell_evaluator.h>
#include <../legalizer/legalizer.h>
namespace circuit {

bool Netlist::cluster_cells(int id1, int id2){
    const design::Design &design = design::Design::get_instance();    
    if(id1 == id2){
        std::cout<<" Error: Cannot cluster the same cell\n";
        return false;
    }
    if(id1 > id2) {
        std::swap(id1, id2);
    }
    const Cell& cell1 = get_cell(id1);
    const Cell& cell2 = get_cell(id2);
    if(cell1.is_clustered() || cell2.is_clustered()){
        std::cout<<"CLSUTER:: Cannot cluster a cell that is already clustered" << id1 << " " << id2 << std::endl;
        return false;
    }

    // TODO:merge cell2 into cell1
    // get cell1 and cell2 origin lib_cell
    int lib_cell_id1 = cell1.get_lib_cell_id();
    int lib_cell_id2 = cell2.get_lib_cell_id();
    const design::LibCell &lib_cell1 = design.get_lib_cell(lib_cell_id1);
    const design::LibCell &lib_cell2 = design.get_lib_cell(lib_cell_id2);
    int new_bits = lib_cell1.get_bits() + lib_cell2.get_bits();
    // Get best cost flip-flop
    const estimator::FFLibcellCostManager &ff_libcell_cost_manager = estimator::FFLibcellCostManager::get_instance();
    int best_lib_cell_id = ff_libcell_cost_manager.get_best_total_cost_lib_cell_id(new_bits);
    
    if(best_lib_cell_id == -1){
        std::cout<<"CLSUTER:: Cannot find a flip-flop with " << new_bits << " bits\n";
        return false;
    }


    double sum_of_origin_cost = ff_libcell_cost_manager.get_lib_cell_cost(lib_cell_id1) + ff_libcell_cost_manager.get_lib_cell_cost(lib_cell_id2);
    double if_cluster_cost = ff_libcell_cost_manager.get_lib_cell_cost(best_lib_cell_id);
    if(sum_of_origin_cost < if_cluster_cost){
        std::cout<<"CLSUTER:: Cannot cluster cells because the sum of origin cost is less than the cluster cost" << std::endl;
        std::cout<<"CLSUTER:: sum_of_origin_cost: "<<sum_of_origin_cost<<" if_cluster_cost: "<<if_cluster_cost<<std::endl;
        return false;
    }else{
        std::cout<<"CLSUTER:: sum_of_origin_cost: "<<sum_of_origin_cost<<" if_cluster_cost: "<<if_cluster_cost<<std::endl;
    }
    // MUST pass previous checks and call modify_circuit_since_merge_cell
    modify_circuit_since_merge_cell(id1, id2, best_lib_cell_id);  
    return true;
}


void Netlist::modify_circuit_since_merge_cell(int id1, int id2, const int new_lib_cell_id){
    const std::string &cell_name1 = get_cell_name(id1);
    const std::string &cell_name2 = get_cell_name(id2);
    std::cout<<"CLUSTER:: modify_circuit_since_merge_cell: "<<cell_name1<<" "<<cell_name2<<std::endl;
    const design::Design &design = design::Design::get_instance();
    Cell& cell1 = get_mutable_cell(id1);
    Cell& cell2 = get_mutable_cell(id2);

    // set cell1 to new_lib_cell
    cell1.set_lib_cell_id(new_lib_cell_id);

    // cell1 new location 
    // TODO: aummption: center of cell1 and cell2
    int new_cell_x = (cell1.get_x() + cell2.get_x()) / 2;
    int new_cell_y = (cell1.get_y() + cell2.get_y()) / 2;
    cell1.set_x(new_cell_x);
    cell1.set_y(new_cell_y);
    // change cell1 width and height
    const design::LibCell &new_lib_cell = design.get_lib_cell(new_lib_cell_id);
    std::cout<<"CLUSTER:: new_lib_cell: "<<new_lib_cell.get_name()<<std::endl;
    cell1.set_w(new_lib_cell.get_width());
    cell1.set_h(new_lib_cell.get_height());
    
    // set cell2 parent to cell1
    cell2.set_parent(id1);
    
    // handle pins mapping
    // need to update pin name, pin location from lib_cell
    // input
    std::vector<std::string> new_lib_cell_input_pins_name = new_lib_cell.get_input_pins_name();
    std::vector<std::pair<double, double>> new_lib_cell_input_pins_position = new_lib_cell.get_input_pins_position();
    std::vector<int> cell1_input_pins_id = cell1.get_input_pins_id();
    std::vector<int> cell2_input_pins_id = cell2.get_input_pins_id();
    std::vector<int> new_cell_input_pins_id;
    new_cell_input_pins_id.reserve(cell1_input_pins_id.size() + cell2_input_pins_id.size());
    new_cell_input_pins_id.insert(new_cell_input_pins_id.end(), cell1_input_pins_id.begin(), cell1_input_pins_id.end());   
    new_cell_input_pins_id.insert(new_cell_input_pins_id.end(), cell2_input_pins_id.begin(), cell2_input_pins_id.end());
    std::cout<<"CLUSTER:: new_cell_input_pins_id check "<<new_cell_input_pins_id.size()<<std::endl;
    for(int i = 0; i < static_cast<int>(new_cell_input_pins_id.size()); i++){
        int pin_id = new_cell_input_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);        
        pin.set_offset_x(new_lib_cell_input_pins_position.at(i).first);
        pin.set_offset_y(new_lib_cell_input_pins_position.at(i).second);
        pin.set_x(new_cell_x + new_lib_cell_input_pins_position.at(i).first);
        pin.set_y(new_cell_y + new_lib_cell_input_pins_position.at(i).second);
        pin.set_cell_id(id1);
    }
    std::cout<<"CLUSTER:: new_cell_input_pins_id: "<<new_cell_input_pins_id.size()<<std::endl;    
    // output
    std::vector<std::string> new_lib_cell_output_pins_name = new_lib_cell.get_output_pins_name();
    std::vector<std::pair<double, double>> new_lib_cell_output_pins_position = new_lib_cell.get_output_pins_position();
    std::vector<int> cell1_output_pins_id = cell1.get_output_pins_id();
    std::vector<int> cell2_output_pins_id = cell2.get_output_pins_id();    
    std::vector<int> new_cell_output_pins_id;
    new_cell_output_pins_id.reserve(cell1_output_pins_id.size() + cell2_output_pins_id.size());
    new_cell_output_pins_id.insert(new_cell_output_pins_id.end(), cell1_output_pins_id.begin(), cell1_output_pins_id.end());
    new_cell_output_pins_id.insert(new_cell_output_pins_id.end(), cell2_output_pins_id.begin(), cell2_output_pins_id.end());
    std::cout<<"CLUSTER:: new_cell_output_pins_id check "<<new_cell_output_pins_id.size()<<std::endl;
    std::cout<<"CLUSTER:: cell1_output_pins_id check "<<cell1_output_pins_id.size()<<std::endl;
    std::cout<<"CLUSTER:: cell2_output_pins_id check "<<cell2_output_pins_id.size()<<std::endl;


    for(int i = 0; i < static_cast<int>(new_cell_output_pins_id.size()); i++){
        int pin_id = new_cell_output_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        pin.set_offset_x(new_lib_cell_output_pins_position.at(i).first);
        pin.set_offset_y(new_lib_cell_output_pins_position.at(i).second);
        pin.set_x(new_cell_x + new_lib_cell_output_pins_position.at(i).first);
        pin.set_y(new_cell_y + new_lib_cell_output_pins_position.at(i).second);
        pin.set_cell_id(id1);
    }
    std::cout<<"CLUSTER:: new_cell_output_pins_id: "<<new_cell_output_pins_id.size()<<std::endl;
    // OTHER PINS LOCATION and OWNER doesn't matter    
    const std::vector<std::pair<double, double>> &new_lib_cell_other_pins_position = new_lib_cell.get_other_pins_position();
    std::vector<int> cell1_other_pins_id = cell1.get_other_pins_id();
    std::vector<int> cell2_other_pins_id = cell2.get_other_pins_id();
    std::vector<int> new_cell_other_pins_id;
    new_cell_other_pins_id.reserve(cell1_other_pins_id.size() + cell2_other_pins_id.size());
    new_cell_other_pins_id.insert(new_cell_other_pins_id.end(), cell1_other_pins_id.begin(), cell1_other_pins_id.end());
    new_cell_other_pins_id.insert(new_cell_other_pins_id.end(), cell2_other_pins_id.begin(), cell2_other_pins_id.end());
    
    // update cell1 pins
    cell1.set_input_pins_id(new_cell_input_pins_id);
    cell1.set_output_pins_id(new_cell_output_pins_id);
    cell1.set_other_pins_id(new_cell_other_pins_id);
    // clear cell2 pins
    cell2.clear();
    remove_sequential_cell(id2);

    std::cout<<"CLUSTER:: legalizer"<<std::endl;
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();
    std::cout<<"CLUSTER:: remove_cell"<<std::endl;
    legalizer.remove_cell(id2);
    std::cout<<"CLUSTER:: replacement_cell"<<std::endl;
    legalizer.replacement_cell(id1);
    legalizer.legalize();    
    
    // update timing information
    // cell1 q_pin_delay change
    std::cout<<"CLUSTER:: update_timing"<<std::endl;
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing(id1);
}

bool Netlist::check_overlap(){
    for(int i = 0; i < static_cast<int>(cells.size()); i++){
        if(cells.at(i).get_parent() != -1){
                continue;
        }

        for(int j = i + 1; j < static_cast<int>(cells.size()); j++){
            if(cells.at(j).get_parent() != -1){
                continue;
            }
            if(cells.at(i).overlap(cells.at(j))){
                return true;
            }
        }
    }
    return false;
}

bool Netlist::check_out_of_die(){
    const design::Design &design = design::Design::get_instance();
    std::vector<double> die_boundaries = design.get_die_boundaries();
    std::cout<<"OUTOFDIE:: die_boundaries: ";
    for(double boundary: die_boundaries){
        std::cout<<boundary<<" ";
    }
    std::cout<<std::endl;

    for(auto &cell: cells){
        if(cell.get_parent() != -1){
            continue;
        }
        if(cell.get_x() < die_boundaries.at(0) || cell.get_rx() > die_boundaries.at(2)){
            print_cell_info(cell);
            return true;
        }
        if(cell.get_y() < die_boundaries.at(1) || cell.get_ry() > die_boundaries.at(3)){
            print_cell_info(cell);
            return true;
        }
    }
    return false;
}

void Netlist::print_cell_info(const Cell &cell){
    const design::Design &design = design::Design::get_instance();
    const design::LibCell &lib_cell = design.get_lib_cell(cell.get_lib_cell_id());
    std::cout<<"Cell: "<<get_cell_name(cell.get_id())<<" LibCell: "<<lib_cell.get_name()<<" x: "<<cell.get_x()<<" y: "<<cell.get_y()<<" rx: "<<cell.get_rx()<<" ry: "<<cell.get_ry()<<std::endl;

}

bool Netlist::decluster_cells(int id){
    const design::Design &design = design::Design::get_instance();    
    const Cell& cell = get_cell(id);
    if(cell.is_clustered()){
        std::cout<<"DECLSUTER:: Cannot cluster a cell that is already clustered" << id<< std::endl;
        return false;
    }else{
        std::cout<<"DECLSUTER:: decluster cell: "<< get_cell_name(id)<<std::endl;
    }
    
    int lib_cell_id = cell.get_lib_cell_id();    
    const design::LibCell &lib_cell = design.get_lib_cell(lib_cell_id);
    int origin_bits = lib_cell.get_bits();
    if(origin_bits == 1){
        std::cout<<"DECLSUTER:: Cannot decluster a cell that is already a single flip-flop" << id<< std::endl;
        return false;
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
        return false;
    }
    if(best_lib_cell_id2 == -1){
        std::cout<<"DECLSUTER:: Cannot find a flip-flop with " << new_bits2 << " bits\n";
        return false;
    }

    double sum_of_origin_cost = ff_libcell_cost_manager.get_lib_cell_cost(lib_cell_id);
    double if_cluster_cost = ff_libcell_cost_manager.get_lib_cell_cost(best_lib_cell_id1) + ff_libcell_cost_manager.get_lib_cell_cost(best_lib_cell_id2);

    const design::LibCell &new_lib_cell1 = design.get_lib_cell(best_lib_cell_id1);
    const design::LibCell &new_lib_cell2 = design.get_lib_cell(best_lib_cell_id2);
    std::cout<<"DECLUSTER:: origin_lib_cell: "<<lib_cell.get_name()<<std::endl;
    std::cout<<"DECLUSTER:: best_lib_cell_id1: <<best_lib_cell_id1: "<<best_lib_cell_id1<<" best_lib_cell_id2: "<<best_lib_cell_id2<<std::endl;
    std::cout<<"DECLUSTER:: new_lib_cell1: "<<new_lib_cell1.get_name()<< " new_lib_cell2: "<<new_lib_cell2.get_name()<<std::endl;
    std::cout<<"DECLSUTER:: sum_of_origin_cost: "<<sum_of_origin_cost<<" if_cluster_cost: "<<if_cluster_cost<<std::endl;

    // MUST pass previous checks and call modify_circuit_since_merge_cell    
    modify_circuit_since_divide_cell(id, best_lib_cell_id1, best_lib_cell_id2);
    return true;
}

void Netlist::modify_circuit_since_divide_cell(int cell_id, const int new_lib_cell_id1, const int new_lib_cell_id2){    
    // add new cell to cells
    std::string new_cell_name = "C" + std::to_string(cells.size());
    Cell temp;
    add_cell(temp,new_cell_name);
    std::cout<<"Add new cell: "<<new_cell_name<<" to netlist"<<std::endl;
    int new_cell_id = temp.get_id();
    std::cout<<"DECLUSTER:: new_cell_id: "<<new_cell_id<<std::endl;
    Cell &new_cell = get_mutable_cell(new_cell_id);

    const design::Design &design = design::Design::get_instance();
    const std::string &cell_name = get_cell_name(cell_id);
    std::cout<<"DECLUSTER:: modify_circuit_since_divide_cell: "<<cell_name << " cell_id:"<< cell_id << std::endl;    
    // original cell become cell1
    Cell& cell1 = get_mutable_cell(cell_id);
    // create new cell    
    // set location, lib_cell_id, width, height
    cell1.set_lib_cell_id(new_lib_cell_id1);
    new_cell.set_lib_cell_id(new_lib_cell_id2);
    std::cout<<"DECLUSTER:: cell1 lib_cell_id: "<<cell1.get_lib_cell_id()<<" cells[cell_id].get_lib_cell_id: "<<cells[cell_id].get_lib_cell_id()<<std::endl;    
    // cell1 new location 
    // TODO: aummption: new_cell is at the origin of cell1 let legalizer handle the location
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

    std::cout<<"DECLUSTER:: origin_cell_input_pins_id check "<<cell1.get_input_pins_id().size()<<std::endl;
    std::cout<<"DECLUSTER:: origin_cell_input_pins_id check "<<cells[cell_id].get_input_pins_id().size()<<std::endl;

    // handle pins mapping
    // give first half of the pins to cell1 and the rest to new_cell    
    int bits1 = new_lib_cell1.get_bits();
    int bits2 = new_lib_cell2.get_bits();
    int origin_bits = bits1 + bits2;

    // INPUT PINS
    std::cout<<"Start input pins"<<std::endl;
    const std::vector<std::pair<double, double>> &new_lib_cell1_input_pins_position = new_lib_cell1.get_input_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell2_input_pins_position = new_lib_cell2.get_input_pins_position();

    std::vector<int> origin_cell_input_pins_id = cell1.get_input_pins_id();
    
    std::cout<<"DECLUSTER:: origin_cell_input_pins_id check "<<cell1.get_input_pins_id().size()<<std::endl;
    std::cout<<"DECLUSTER:: origin_cell_input_pins_id check "<<cells[cell_id].get_input_pins_id().size()<<std::endl;

    std::vector<int> new_cell_input_pins_id1, new_cell_input_pins_id2;
    new_cell_input_pins_id1.reserve(bits1);
    new_cell_input_pins_id2.reserve(bits2);
    new_cell_input_pins_id1.insert(new_cell_input_pins_id1.end(), origin_cell_input_pins_id.begin(), origin_cell_input_pins_id.begin() + bits1);
    new_cell_input_pins_id2.insert(new_cell_input_pins_id2.end(), origin_cell_input_pins_id.begin() + bits1, origin_cell_input_pins_id.end());
    std::cout<<"DECLUSTER:: new_cell_input_pins_id1 check "<<new_cell_input_pins_id1.size()<<std::endl;
    std::cout<<"DECLUSTER:: new_cell_input_pins_id2 check "<<new_cell_input_pins_id2.size()<<std::endl;

    for(int i = 0; i < origin_bits; i++){
        int pin_id = origin_cell_input_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        int new_x_offset,new_y_offset;
        if(i < bits1){
            // origin
            new_x_offset = new_lib_cell1_input_pins_position.at(i).first;
            new_y_offset = new_lib_cell1_input_pins_position.at(i).second;        
            std::cout<<"DECLUSTER:: pin_id "<<pin_id<<" cell_id"<<pin.get_cell_id()<<std::endl;
        }else{
            // new
            new_x_offset = new_lib_cell2_input_pins_position.at(i - bits1).first;
            new_y_offset = new_lib_cell2_input_pins_position.at(i - bits1).second;
            pin.set_cell_id(new_cell_id);
            std::cout<<"DECLUSTER:: pin_id "<<pin_id<<" cell_id"<<pin.get_cell_id()<<std::endl;
        }
        pin.set_x(new_cell_x + new_x_offset);
        pin.set_y(new_cell_y + new_y_offset);
    }
    cell1.set_input_pins_id(new_cell_input_pins_id1);
    cells.at(new_cell_id).set_input_pins_id(new_cell_input_pins_id2);

    // OUTPUT PINS    
    const std::vector<std::pair<double, double>> &new_lib_cell1_output_pins_position = new_lib_cell1.get_output_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell2_output_pins_position = new_lib_cell2.get_output_pins_position();
    std::vector<int> origin_cell_output_pins_id = cell1.get_output_pins_id();
    std::vector<int> new_cell_output_pins_id1, new_cell_output_pins_id2;
    new_cell_output_pins_id1.reserve(bits1);
    new_cell_output_pins_id2.reserve(bits2);
    new_cell_output_pins_id1.insert(new_cell_output_pins_id1.end(), origin_cell_output_pins_id.begin(), origin_cell_output_pins_id.begin() + bits1);
    new_cell_output_pins_id2.insert(new_cell_output_pins_id2.end(), origin_cell_output_pins_id.begin() + bits1, origin_cell_output_pins_id.end());
    std::cout<<"DECLUSTER:: new_cell_output_pins_id1 check "<<new_cell_output_pins_id1.size()<<std::endl;
    std::cout<<"DECLUSTER:: new_cell_output_pins_id2 check "<<new_cell_output_pins_id2.size()<<std::endl;

    for(int i = 0; i < origin_bits; i++){
        int pin_id = origin_cell_output_pins_id.at(i);
        Pin &pin = get_mutable_pin(pin_id);
        int new_x_offset,new_y_offset;
        if(i < bits1){
            // origin
            new_x_offset = new_lib_cell1_output_pins_position.at(i).first;
            new_y_offset = new_lib_cell1_output_pins_position.at(i).second;
            std::cout<<"DECLUSTER:: pin_id "<<pin_id<<" cell_id"<<pin.get_cell_id()<<std::endl;
        }else{
            // new
            new_x_offset = new_lib_cell2_output_pins_position.at(i - bits1).first;
            new_y_offset = new_lib_cell2_output_pins_position.at(i - bits1).second;
            pin.set_cell_id(new_cell_id);
            std::cout<<"DECLUSTER:: pin_id "<<pin_id<<" cell_id"<<pin.get_cell_id()<<std::endl;
        }
        pin.set_x(new_cell_x + new_x_offset);
        pin.set_y(new_cell_y + new_y_offset);       
    }
    cell1.set_output_pins_id(new_cell_output_pins_id1);
    cells.at(new_cell_id).set_output_pins_id(new_cell_output_pins_id2);

    // OTHER PINS LOCATION and OWNER doesn't matter
    const std::vector<std::pair<double, double>> &new_lib_cell1_other_pins_position = new_lib_cell1.get_other_pins_position();
    const std::vector<std::pair<double, double>> &new_lib_cell2_other_pins_position = new_lib_cell2.get_other_pins_position();
    const std::vector<int> &origin_cell1_other_pins_id = cell1.get_other_pins_id();

    int cell1_other_pins_size = new_lib_cell1_other_pins_position.size();
    int cell2_other_pins_size = new_lib_cell2_other_pins_position.size();
    std::vector<int> new_cell1_other_pins_id, new_cell2_other_pins_id;
    new_cell1_other_pins_id.reserve(cell1_other_pins_size);
    new_cell2_other_pins_id.reserve(cell2_other_pins_size);
    
    for(int pin_id : origin_cell1_other_pins_id){
        new_cell1_other_pins_id.push_back(pin_id);
        new_cell2_other_pins_id.push_back(pin_id);
    }

    cell1.set_other_pins_id(new_cell1_other_pins_id);
    cells.at(new_cell_id).set_other_pins_id(new_cell2_other_pins_id);
    
    add_sequential_cell(new_cell_id);
    
    std::cout<<"DECLUSTER:: legalizer"<<std::endl;
    legalizer::Legalizer &legalizer = legalizer::Legalizer::get_instance();     
    legalizer.replacement_cell(cell_id);
    legalizer.add_cell(new_cell_id);
    legalizer.legalize();

    // update timing information
    // cell1 new_cell q_pin_delay, location change
    std::cout<<"DECLUSTER:: update_timing"<<std::endl;
    timer::Timer &timer = timer::Timer::get_instance();
    timer.update_timing(cell_id);
    timer.update_timing(new_cell_id);
}


} // namespace circuit