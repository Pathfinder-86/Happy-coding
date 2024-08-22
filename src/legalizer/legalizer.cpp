#include "legalizer.h"
#include "../circuit/netlist.h"
#include "../circuit/original_netlist.h"
#include "../circuit/cell.h"
#include <unordered_set>
#include <algorithm>
#include <climits>
#include <../runtime/runtime.h>
#include <iostream>

namespace legalizer{

bool Legalizer::check_on_site(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(const circuit::Cell &cell : cells){
        if(cell.is_clustered()){
            continue;
        }
        int x = cell.get_x();
        int y = cell.get_y();
        if(sites_xy_to_id_map.find(std::make_pair(x,y)) == sites_xy_to_id_map.end()){
            int cid = cell.get_id();            
            std::cerr << "Cell " << cid << "(x=" << x << ",y=" << y << ") is not on site." << std::endl;
            return false;
        }                                                
    }
    return true;
}

void Legalizer::init_blockage(){
    const circuit::OriginalNetlist &netlist = circuit::OriginalNetlist::get_instance();
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

        // Binary search to find the starting row index
        int start = 0;
        int end = rows.size() - 1;
        int startRowIdx = -1;
        while (start <= end) {
            int mid = start + (end - start) / 2;
            if (rows[mid].get_y() <= y && rows[mid].get_ry() > y) {
                startRowIdx = mid;
                break;
            } else if (rows[mid].get_y() >= y) {
                end = mid - 1;
            } else {
                start = mid + 1;
            }
        }

        // Binary search to find the ending row index
        start = 0;
        end = rows.size() - 1;
        int endRowIdx = -1;
        while (start <= end) {
            int mid = start + (end - start) / 2;
            if (rows[mid].get_y() < ry && rows[mid].get_ry() >= ry) {
                endRowIdx = mid;
                break;
            } else if (rows[mid].get_y() >= ry) {
                end = mid - 1;
            } else {
                start = mid + 1;
            }
        }

        // Remove x overlapping sites in the rows
        for (int i = startRowIdx; i <= endRowIdx; i++) {
            int row_id = rows[i].get_id();
            const std::vector<int> &sites_id = row_id_to_sites_id_map.at(row_id);
            // sites is sorted by x using binary search to find the starting site index
            start = 0;
            end = sites_id.size() - 1;
            int startSiteIdx = -1;
            while (start <= end) {
                int mid = start + (end - start) / 2;
                int site_id = sites_id[mid];
                if (sites[site_id].get_x() <= x && sites[site_id].get_rx() > x) {
                    startSiteIdx = mid;
                    break;
                } else if (sites.at(sites_id[mid]).get_x() >= x) {
                    end = mid - 1;
                } else {
                    start = mid + 1;
                }
            }


            // remove site until site.x >= rx
            for(int i=startSiteIdx;i<sites_id.size();i++){
                int site_id = sites_id.at(i);
                Site &site = sites.at(site_id);
                if(site.get_x() >= rx){
                    break;
                }
                site.set_empty(false);
            }

        }
    }
    // ignore sites been blocked by combinational cells
    std::vector<Site> new_sites;
    for(const Site &site : sites){
        if(site.is_empty()){
            new_sites.push_back(site);
        }
    }
    sites = new_sites;
    sort_sites_by_y_then_x();
    // adjust site_id since some sites are removed
    for(int i=0;i<sites.size();i++){
        sites.at(i).set_id(i);
    }

    for(const auto &it : sites){        
        empty_sites_id.insert(it.get_id());
        sites_id_to_xy_map[it.get_id()] = std::make_pair(it.get_x(),it.get_y());
        sites_xy_to_id_map[std::make_pair(it.get_x(),it.get_y())] = it.get_id();
    }
}

void Legalizer::sort_rows_by_y(){
    std::sort(rows.begin(),rows.end(),[](const Row &a, const Row &b){
        return a.get_y() < b.get_y();
    });
}

void Legalizer::sort_sites_by_y_then_x(){
    std::sort(sites.begin(),sites.end(),[](const Site &a, const Site &b){
        if(a.get_y() == b.get_y()){
            return a.get_x() < b.get_x();
        }
        return a.get_y() < b.get_y();
    });
}

int Legalizer::nearest_empty_site(int x, int y) const{
    auto it = sites_xy_to_id_map.find(std::make_pair(x,y));
    if(it != sites_xy_to_id_map.end() && empty_sites_id.count(it->second)){
        return it->second;
    }
        
    int min_dist = INT_MAX;
    int nearest_site_id = -1;

    for(int site_id : empty_sites_id){
        //std::cout<<"LEGAL:: empty_sites_id site_id "<<site_id<<std::endl;
        const Site &site = sites.at(site_id);
        int sx = site.get_x();
        int sy = site.get_y();
        int dist = abs(x - sx) + abs(y - sy);
        if(dist < min_dist){
            min_dist = dist;
            nearest_site_id = site_id;
        }
    }
    //std::cout<<"LEGAL:: nearest_empty_site nearest_site_id "<<nearest_site_id<<std::endl;    
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

    int x_site_num = ((rx - x) % get_site_width() == 0)? (rx - x) / get_site_width() : (rx - x) / get_site_width() + 1;
    int y_site_num = ((ry - y) % get_site_height() == 0)? (ry - y) / get_site_height() : (ry - y) / get_site_height() + 1;
    std::vector<int> ret_sites_id;    
    return try_extend_at_multiple_sites_id(empty_sites_sort_by_distance,x_site_num,y_site_num,ret_sites_id)?  ret_sites_id : std::vector<int>();
}

std::vector<int> Legalizer::nearest_empty_site_enough_space(int x, int y, int rx, int ry){
    //std::cout<<"LEGAL:: nearest_empty_site_enough_space START"<<std::endl;
    int nearest_site_id = nearest_empty_site(x,y);
    //std::cout<<"LEGAL:: nearest_empty_site_enough_space nearest_site_id "<<nearest_site_id<<std::endl;
    if(nearest_site_id == -1){
        return {};
    }
    //std::cout<<"LEGAL:: x "<<x<<" y "<<y<<" rx "<<rx<<" ry "<<ry<<std::endl;
    // TODO:
    int x_site_num = ((rx - x) % get_site_width() == 0)? (rx - x) / get_site_width() : (rx - x) / get_site_width() + 1;
    int y_site_num = ((ry - y) % get_site_height() == 0)? (ry - y) / get_site_height() : (ry - y) / get_site_height() + 1;
    //std::cout<<"LEGAL:: x_site_num "<<x_site_num<<" y_site_num "<<y_site_num<<std::endl;
    std::vector<int> sites_id;
    return extend_at_site_id(nearest_site_id,sites_id,x_site_num,y_site_num) ? sites_id : std::vector<int>();
}

void Legalizer::init(){
    const runtime::RuntimeManager &runtime_manager = runtime::RuntimeManager::get_instance();
    std::cout<<"LEGAL:: init"<<std::endl;
    runtime_manager.get_runtime();    
    sort_rows_by_y();
    init_blockage();
    std::cout<<"LEGAL:: init_blockage finish"<<std::endl;
    runtime_manager.get_runtime();    
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    not_on_site_cells_id = netlist.get_sequential_cells_id();
    place_available_cells_on_empty_sites_sort_by_slack();    
    move_unavailable_cells_to_empty_sites_sort_by_slack();
    std::cout<<"LEGAL:: finish"<<std::endl;
    runtime_manager.get_runtime();    
}

bool Legalizer::extend_at_site_id(const int root_site_id,std::vector<int> &sites_id,int x_site_num,int y_site_num){  
    const Site &site = sites.at(root_site_id);      
    int x = site.get_x();
    int y = site.get_y();
    
    for(int i=0;i<x_site_num;i++){
        for(int j=0;j<y_site_num;j++){
            int new_x = x + i* get_site_width();
            int new_y = y + j* get_site_height();
            auto it = sites_xy_to_id_map.find(std::make_pair(new_x,new_y));
            if(it == sites_xy_to_id_map.end()){
                return false;
            }
            int new_site_id = it->second;
            if(!empty_sites_id.count(new_site_id)){
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
                int new_x = x + i*get_site_width();
                int new_y = y + j*get_site_height();
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
        }else{
            ret_sites_id.clear();
        }
    }
    return false;
}

void Legalizer::remove_cell(int cell_id){
    not_on_site_cells_id.erase(cell_id);
    auto it = cell_id_to_site_id_map.find(cell_id);
    if(it == cell_id_to_site_id_map.end()){
        return;
    }     
    const std::vector<int> &sites_id = it->second;    
    for(int site_id : sites_id){
        empty_sites_id.insert(site_id);
        site_id_to_cell_id_map.erase(site_id);
    } 
    cell_id_to_site_id_map.erase(cell_id);
}

void Legalizer::replacement_cell(int cell_id){        
    const std::vector<int> &sites_id = cell_id_to_site_id_map.at(cell_id);
    for(int site_id : sites_id){
        empty_sites_id.insert(site_id);
        site_id_to_cell_id_map.erase(site_id);
    }
    cell_id_to_site_id_map.erase(cell_id);
    not_on_site_cells_id.insert(cell_id);
}


void Legalizer::add_cell(int cell_id){
    not_on_site_cells_id.insert(cell_id);
}


void Legalizer::place_available_cells_on_empty_sites(){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::vector<circuit::Cell> &cells = netlist.get_mutable_cells();            
    for(auto it = not_on_site_cells_id.begin(); it != not_on_site_cells_id.end();){
        int cid = *it;
        circuit::Cell &cell = cells.at(cid);    
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();   
        //std::cout<<"LEGAL:: Try to place cell "<<netlist.get_cell_name(cid)<<"origin at ("<<x<<","<<y<<")"<<std::endl;
        const std::vector<int> &sites_id = nearest_empty_site_enough_space(x,y,rx,ry);
        if(!sites_id.empty()){
            it = not_on_site_cells_id.erase(it);
            int site_id = sites_id.at(0);
            const Site &site = sites.at(site_id);            
            int site_x = site.get_x();
            int site_y = site.get_y();
            if(site_x != x || site_y != y){
                //std::cout<<"LEGAL:: MOVE to nearest cell "<<netlist.get_cell_name(cid)<< " at ("<<site_x<<","<<site_y<<")"<<std::endl;
                cell.move(site_x,site_y);
            }else{
                //std::cout<<"LEGAL:: cell "<<netlist.get_cell_name(cid)<< " don't need to move"<<std::endl;
            }

            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].push_back(site_id);
                //std::cout<<"LEGAL:: add site_id "<<site_id<<std::endl;
            }                           
        }else{
            it++;
            //std::cout<<"LEGAL:: place_available_cells_on_empty_sites FAIL"<<std::endl;
        }
    }
}

void Legalizer::place_available_cells_on_empty_sites_sort_by_slack(){
    // sort by slack first
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::vector<circuit::Cell> &cells = netlist.get_mutable_cells();     
    std::vector<int> sorted_not_on_site_cells_id;
    sorted_not_on_site_cells_id.reserve(not_on_site_cells_id.size());
    for(int cid : not_on_site_cells_id){
        sorted_not_on_site_cells_id.push_back(cid);
    }
    std::sort(sorted_not_on_site_cells_id.begin(),sorted_not_on_site_cells_id.end(),[&](int a, int b){
        return cells.at(a).get_slack() < cells.at(b).get_slack();
    });
    for(int cid : sorted_not_on_site_cells_id){        
        circuit::Cell &cell = cells.at(cid);    
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();           
        const std::vector<int> &sites_id = nearest_empty_site_enough_space(x,y,rx,ry);
        if(!sites_id.empty()){
            not_on_site_cells_id.erase(cid);
            int site_id = sites_id.at(0);
            const Site &site = sites.at(site_id);            
            int site_x = site.get_x();
            int site_y = site.get_y();
            if(site_x != x || site_y != y){
                //std::cout<<"LEGAL:: MOVE to nearest cell "<<netlist.get_cell_name(cid)<< " at ("<<site_x<<","<<site_y<<")"<<std::endl;
                cell.move(site_x,site_y);
            }else{
                //std::cout<<"LEGAL:: cell "<<netlist.get_cell_name(cid)<< " don't need to move"<<std::endl;
            }

            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].push_back(site_id);
                //std::cout<<"LEGAL:: add site_id "<<site_id<<std::endl;
            }                           
        }
    }
}

void Legalizer::move_unavailable_cells_to_empty_sites(){    
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::vector<circuit::Cell> &cells = netlist.get_mutable_cells();
    runtime::RuntimeManager &runtime_manager = runtime::RuntimeManager::get_instance();    
    for(auto it = not_on_site_cells_id.begin(); it != not_on_site_cells_id.end();){
        int cid = *it;
        circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();
        //std::cout<<"LEGAL:: Try to place cell "<<netlist.get_cell_name(cid)<<"origin at ("<<x<<","<<y<<")"<<std::endl;
        const std::vector<int> &sites_id = empty_sites_enough_space(x,y,rx,ry);        
        if(!sites_id.empty()){
            it = not_on_site_cells_id.erase(it);
            int site_id = sites_id.at(0);
            const Site &site = sites.at(site_id);            
            int site_x = site.get_x();
            int site_y = site.get_y();
            if(site_x != x || site_y != y){
                //std::cout<<"LEGAL:: MOVE to NONnearest cell "<<netlist.get_cell_name(cid)<< " at ("<<site_x<<","<<site_y<<")"<<std::endl;
                cell.move(site_x,site_y);
            }else{
                //std::cout<<"LEGAL:: cell "<<netlist.get_cell_name(cid)<< " don't need to move"<<std::endl;
            }

            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].push_back(site_id);
            }                           
        }else{
            it++;
            //std::cout<<"LEGAL:: move_unavailable_cells_to_empty_sites FAIL"<<std::endl;
        }
    }
}

void Legalizer::move_unavailable_cells_to_empty_sites_sort_by_slack(){
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
    std::vector<circuit::Cell> &cells = netlist.get_mutable_cells();
    std::vector<int> sorted_not_on_site_cells_id;
    sorted_not_on_site_cells_id.reserve(not_on_site_cells_id.size());
    for(int cid : not_on_site_cells_id){
        sorted_not_on_site_cells_id.push_back(cid);
    }
    std::sort(sorted_not_on_site_cells_id.begin(),sorted_not_on_site_cells_id.end(),[&](int a, int b){
        return cells.at(a).get_slack() < cells.at(b).get_slack();
    });    
    for(int cid : sorted_not_on_site_cells_id){        
        circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();
        const std::vector<int> &sites_id = empty_sites_enough_space(x,y,rx,ry);        
        if(!sites_id.empty()){
            not_on_site_cells_id.erase(cid);
            int site_id = sites_id.at(0);
            const Site &site = sites.at(site_id);            
            int site_x = site.get_x();
            int site_y = site.get_y();
            if(site_x != x || site_y != y){
                //std::cout<<"LEGAL:: MOVE to NONnearest cell "<<netlist.get_cell_name(cid)<< " at ("<<site_x<<","<<site_y<<")"<<std::endl;
                cell.move(site_x,site_y);
            }else{
                //std::cout<<"LEGAL:: cell "<<netlist.get_cell_name(cid)<< " don't need to move"<<std::endl;
            }

            for(int site_id : sites_id){
                empty_sites_id.erase(site_id);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid].push_back(site_id);
            }                           
        }
    }
}


void Legalizer::print_empty_sites() const{
    for(int site_id : empty_sites_id){
        const Site &site = sites.at(site_id);
        //std::cout << "Empty site (" << site.get_x() << "," << site.get_y() << ")" << std::endl;
    }
}


bool Legalizer::legalize(){
    if(!available){
        return false;
    }            
    place_available_cells_on_empty_sites_sort_by_slack();
    move_unavailable_cells_to_empty_sites_sort_by_slack();
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    if(!valid()){
        std::cerr << "Legalization failed." << std::endl;
        return false;
    }
    return true;
}

void Legalizer::switch_to_other_solution( const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map){
    this->cell_id_to_site_id_map = cell_id_to_site_id_map;
    // update empty_sites_id
    empty_sites_id.clear(); 
    site_id_to_cell_id_map.clear();
    not_on_site_cells_id.clear(); // legal solution should be empty

    for(int i=0;i<sites.size();i++){
        empty_sites_id.insert(i);
    }
    for(const auto &it : cell_id_to_site_id_map){
        for(int site_id : it.second){
            empty_sites_id.erase(site_id);
            site_id_to_cell_id_map[site_id] = it.first;
        }        
    }    
}

}