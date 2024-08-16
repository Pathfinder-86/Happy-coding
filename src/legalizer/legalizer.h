#ifndef LEGALIZER_H
#define LEGALIZER_H
#include <set>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <map>
namespace legalizer {

/*struct Row {
    Row(int x, int y, int width, int height): x(x), y(y), width(width), height(height){}
    int x,y;
    int width,height;
    int get_x(){
        return x;
    }
    int get_y(){
        return y;
    }
    int get_rx(){
        return x + width;
    }
    int get_ry(){
        return y + height;
    }
};*/

struct Site {
    Site(int x, int y, int width, int height): x(x), y(y), width(width), height(height){}
    int x,y;
    int width,height;
    int get_x() const{
        return x;
    }
    int get_y() const{
        return y;
    }
    int get_rx() const{
        return x + width;
    }
    int get_ry() const{
        return y + height;
    }
};

class Legalizer{
    public:
        Legalizer(): site_height(-1),site_width(-1),available(true){}
        static Legalizer& get_instance() {
            static Legalizer instance;        
            return instance;
        }
        void add_row(int x, int y, int site_width, int site_height, int site_num){
            //rows.push_back(Row{x,y,site_width*site_num,site_height});
            for(int i = 0; i < site_num; i++){
                sites.push_back(Site(x + i*site_width, y, site_width, site_height));
                //std::cout << "Site " << x + i*site_width << "," << y << std::endl;                
            }

            if(site_height == -1){
                site_height = site_height;
                site_width = site_width;
            }else{
                if(site_height != site_height || site_width != site_width){
                    std::cerr << "Site height and width should be the same for all rows" << std::endl;
                    available = false;
                }
            }
        }
        void init();
        void reset(){
            //rows = init_rows;
            sites = init_sites;
        }
        bool check_on_site();
        bool legalize();
        void init_blockage();
        void place_available_cells_on_empty_sites();
        void move_unavailable_cells_to_empty_sites();
        void print_empty_sites() const;        
        int nearest_empty_site(int x, int y) const;
        std::vector<int> nearest_empty_site_enough_space(int x, int y, int rx, int ry);
        std::vector<int> empty_sites_enough_space(int x, int y, int rx, int ry);
        bool extend_at_site_id(std::vector<int> &sites_id,int x_site_num,int y_site_num);
        bool legalize_success() const{
            return not_on_site_cells_id.empty();
        }
        std::vector<int> distance_order_empty_sites(int x, int y);
        bool try_extend_at_multiple_sites_id(const std::vector<int> &empty_sites_id,int x_site_num,int y_site_num,std::vector<int> &ret_sites_id);
    private:   
        //std::vector<Row> rows;
        //std::vector<Row> init_rows;

        std::vector<Site> init_sites;
        // quick site access, check
        std::vector<Site> sites;   
        std::unordered_map<int,std::pair<int,int>> sites_id_to_xy_map;
        std::map<std::pair<int,int>,int> sites_xy_to_id_map;
        std::unordered_set<int> empty_sites_id;

        // site cell relation
        std::unordered_set<int> not_on_site_cells_id;
        std::unordered_map<int,int> site_id_to_cell_id_map;
        std::unordered_map<int,std::unordered_set<int>> cell_id_to_site_id_map;

        int site_width, site_height;
        bool available;
};

}

#endif // LEGALIZER_H