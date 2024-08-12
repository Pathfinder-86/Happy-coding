#include "legalizer.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include <unordered_set>
#include <algorithm>
#include <climits>

namespace legalizer{

bool Legalizer::check_on_site(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cid : sequential_cells_id){
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        if(sites_xy_to_id_map.find(std::make_pair(x,y)) == sites_xy_to_id_map.end()){
            int cid = cell.get_id();
            const std::string &cell_name = netlist.get_cell_name(cid);
            std::cerr << "Cell " << cell_name << "(x=" << x << ",y=" << y << ") is not on site." << std::endl;
            return false;
        }                                                
    }
    return true;
}

void Legalizer::init_blockage(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(const circuit::Cell &cell : cells){
        if(cell.is_sequential()){
            continue;
        }
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();
        // remove overlapping sites
        for(auto it = sites.begin(); it != sites.end();){
            int sx = it->get_x();
            int sy = it->get_y();
            int srx = it->get_rx();
            int sry = it->get_ry();
            // if not overlap
            if(rx <= sx || ry <= sy || x >= srx || y >= sry){
                it++;
                continue;
            }else{
                it = sites.erase(it);
                //std::cout<<"Blockage at site ("<<sx<<","<<sy<<")"<<std::endl;
            }                        
        }
        
    }
    for(int site_id = 0; site_id < sites.size(); site_id++){        
        empty_sites_id.insert(site_id);
        sites_id_to_xy_map[site_id] = std::make_pair(sites.at(site_id).get_x(),sites.at(site_id).get_y());
        sites_xy_to_id_map[std::make_pair(sites.at(site_id).get_x(),sites.at(site_id).get_y())] = site_id; 
    }
}

int Legalizer::nearest_empty_site(int x, int y) const{
    auto it = sites_xy_to_id_map.find(std::make_pair(x,y));
    if(it != sites_xy_to_id_map.end() && empty_sites_id.find(it->second) != empty_sites_id.end()){
        return it->second;
    }
    
    
    int min_dist = INT_MAX;
    int nearest_site_id = -1;

    for(int site_id : empty_sites_id){
        const Site &site = sites.at(site_id);
        int sx = site.get_x();
        int sy = site.get_y();
        int dist = abs(x - sx) + abs(y - sy);
        if(dist < min_dist){
            min_dist = dist;
            nearest_site_id = site_id;
        }
    }
    return nearest_site_id;
}

std::vector<int> Legalizer::distance_order_empty_sites(int x, int y){
    std::vector<std::pair<int,int>> site_id_distance;
    for(int site_id : empty_sites_id){
        const Site &site = sites.at(site_id);
        int sx = site.get_x();
        int sy = site.get_y();
        int dist = abs(x - sx) + abs(y - sy);
        site_id_distance.push_back(std::make_pair(site_id,dist));
    }

    std::sort(site_id_distance.begin(),site_id_distance.end(),[](const std::pair<int,int> &a, const std::pair<int,int> &b){
        return a.second < b.second;
    });

    std::vector<int> site_ids;
    for(const auto &it : site_id_distance){
        site_ids.push_back(it.first);
    }
    return site_ids;
}

std::vector<int> Legalizer::empty_sites_enough_space(int x, int y, int rx, int ry){
    const std::vector<int> &empty_sites_sort_by_distance = distance_order_empty_sites(x,y);
    if(empty_sites_sort_by_distance.empty()){
        return empty_sites_sort_by_distance;
    }

    int x_site_num = (rx - x) / site_width;
    int y_site_num = (ry - y) / site_height;
    std::vector<int> ret_sites_id;
    return try_extend_at_multiple_sites_id(empty_sites_sort_by_distance,x_site_num,y_site_num,ret_sites_id)?  ret_sites_id : std::vector<int>();
}

std::vector<int> Legalizer::nearest_empty_site_enough_space(int x, int y, int rx, int ry){
    int nearest_site_id = nearest_empty_site(x,y);
    if(nearest_site_id == -1){
        return {};
    }
    int x_site_num = (rx - x) / site_width;
    int y_site_num = (ry - y) / site_height;
    std::vector<int> site_ids;
    site_ids.push_back(nearest_site_id);
    return extend_at_site_id(site_ids,x_site_num,y_site_num) ? site_ids : std::vector<int>();
}


bool Legalizer::extend_at_site_id(std::vector<int> &sites_id,int x_site_num,int y_site_num){    
    const Site &site = sites.at(sites_id.at(0));
    int x = site.get_x();
    int y = site.get_y();

    // extend x
    for(int i=0;i<x_site_num;i++){
        for(int j=0;j<y_site_num;j++){
            if(i == 0 && j== 0){
                continue;
            }
            int new_x = x + i*site_width;
            int new_y = y + j*site_height;
            if(sites_xy_to_id_map.find(std::make_pair(new_x,new_y)) == sites_xy_to_id_map.end()){
                return false;
            }
            int new_site_id = sites_xy_to_id_map.at(std::make_pair(new_x,new_y));
            if(empty_sites_id.find(new_site_id) == empty_sites_id.end()){
                return false;
            }
            sites_id.push_back(new_site_id);
        }
    }
        
    return true;
}

bool Legalizer::try_extend_at_multiple_sites_id(const std::vector<int> &root_sites_id,int x_site_num,int y_site_num,std::vector<int> &ret_sites_id){
    
    for(int site_id : root_sites_id){
        const Site &site = sites.at(site_id);
        int x = site.get_x();
        int y = site.get_y();
        bool success = true;
        // extend from site_id
        for(int i=0;i<x_site_num;i++){
            for(int j=0;j<y_site_num;j++){                
                int new_x = x + i*site_width;
                int new_y = y + j*site_height;
                if(sites_xy_to_id_map.find(std::make_pair(new_x,new_y)) == sites_xy_to_id_map.end()){
                    success = false;
                    break;
                }
                int new_site_id = sites_xy_to_id_map.at(std::make_pair(new_x,new_y));
                if(empty_sites_id.find(new_site_id) == empty_sites_id.end()){
                    success = false;
                    break;
                }
                ret_sites_id.push_back(new_site_id);
            }            
            if(!success){
                break;
            }
        }

        if(success){            
            return true;
        }
    }
    return false;
}



void Legalizer::place_available_cells_on_empty_sites(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    not_on_site_cells_id = sequential_cells_id;

    for(auto it = not_on_site_cells_id.begin(); it != not_on_site_cells_id.end();){
        int cid = *it;
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();        
        const std::vector<int> &sites_id = nearest_empty_site_enough_space(x,y,rx,ry);
        if(!sites_id.empty()){
            it = not_on_site_cells_id.erase(it);
            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].insert(site_id);
            }
        }
    }
}

void Legalizer::move_unavailable_cells_to_empty_sites(){    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(auto it = not_on_site_cells_id.begin(); it != not_on_site_cells_id.end();){
        int cid = *it;
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();        
        const std::vector<int> &sites_id = empty_sites_enough_space(x,y,rx,ry);
        if(!sites_id.empty()){
            it = not_on_site_cells_id.erase(it);
            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].insert(site_id);
            }
        }
    }
}


void Legalizer::print_empty_sites() const{
    for(int site_id : empty_sites_id){
        const Site &site = sites.at(site_id);
        std::cout << "Empty site (" << site.get_x() << "," << site.get_y() << ")" << std::endl;
    }
}


bool Legalizer::legalize(){
    if(!available){
        return false;
    }    
    place_available_cells_on_empty_sites();
    move_unavailable_cells_to_empty_sites();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    if(!legalize_success()){
        std::cerr << "Legalization failed." << std::endl;
        for(int cid : not_on_site_cells_id){
            const circuit::Cell &cell = cells.at(cid);
            const std::string &cell_name = netlist.get_cell_name(cid);
            std::cerr << "Cell " << cell_name << "(x=" << cell.get_x() << ",y=" << cell.get_y() << ") cannot be placed." << std::endl;
        }
        return false;
    }
    return true;
}



}