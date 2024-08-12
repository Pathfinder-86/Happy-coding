#include "legalizer.h"
#include "../circuit/netlist.h"
#include "../circuit/cell.h"
namespace legalizer{

bool Legalizer::check_on_site(){
    const circuit::Netlist &netlist = circuit::Netlist::get_instance();
    const std::vector<circuit::Cell> &cells = netlist.get_cells();
    for(const circuit::Cell &cell : cells){
        double x = cell.get_x();
        double y = cell.get_y();                
        if(sites_set.find(std::make_pair(x,y)) == sites_set.end()){
            int cid = cell.get_id();
            const std::string &cell_name = netlist.get_cell_name(cid);
            std::cerr << "Cell " << cell_name << "(x=" << x << ",y=" << y << ") is not on site." << std::endl;
            return false;
        }                                                
    }
    return true;
}




}