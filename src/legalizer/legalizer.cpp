#include "legalizer.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
#include <unordered_set>
#include <algorithm>

namespace legalizer{

bool Legalizer::check_on_site(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    for(int cid : sequential_cells_id){
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        if(sites_xy_to_id_map.find(std::make_pair(x,y)) == on_sites_set.end()){
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

void Legalizer::find_unplaced_cells_and_empty_sites(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    const std::unordered_set<int> &sequential_cells_id = netlist.get_sequential_cells_id();
    not_on_site_cells_id = sequential_cells_id;
    for(int cid : sequential_cells_id){
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();
        // mark overlapping sites empty to false

        for(auto it = empty_sites_id.begin() ; it != empty_sites_id.end();){
            int site_id = *it;
            const Site &site = sites.at(site_id);
            int sx =  site.get_x();
            int sy =  site.get_y();
            int srx = site.get_rx();
            int sry = site.get_ry();
            // if not overlap
            if(rx <= sx || ry <= sy || x >= srx || y >= sry){
                it++;                
                continue;
            }else{
                it = empty_sites_id.erase(it);
                site_id_to_cell_id_map[site_id] = cid;
                cell_id_to_site_id_map[cid] = site_id;
                not_on_site_cells_id.erase(cid);

                //const std::string &cell_name = netlist.get_cell_name(cid);
                //std::cout << "Cell " << cell_name << " at site (" << sx << "," << sy << ")" << std::endl;
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

    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(int cid : not_on_site_cells_id){
        const circuit::Cell &cell = cells.at(cid);
        int x = cell.get_x();
        int y = cell.get_y();
        int rx = cell.get_rx();
        int ry = cell.get_ry();
        // find the nearest empty site
        std::vector<std::pair<int,double>> cell_sites_distances;
        for(int site_id : empty_sites_id){
            const Site &site = sites.at(site_id);
            int sx = site.get_x();
            int sy = site.get_y();
            int srx = site.get_rx();
            int sry = site.get_ry();
            double dist = abs(x - sx) + abs(y - sy);
            cell_sites_distances.push_back(std::make_pair(site_id,dist));
        }

        std::sort(cell_sites_distances.begin(),cell_sites_distances.end(),[](const std::pair<int,double> &a, const std::pair<int,double> &b){
            return a.second < b.second;
        });
        
        if(cell_sites_distances.empty()){
            std::cerr << "No available site for cell " << netlist.get_cell_name(cid) << std::endl;
            return false;
        }
        // cells may larger than sites
        // need mutiple sites to place a cell
        // try to place the cell until success or return false
        for(const auto &it : cell_sites_distances){
            int site_id = it.first;
            const Site &site = sites.at(site_id);
            int sx = site.get_x();
            int sy = site.get_y();
            int srx = site.get_rx();
            int sry = site.get_ry();
            if(rx )
        }

        
    }
}



}