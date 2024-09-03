#ifndef LEGALIZER_H
#define LEGALIZER_H
#include <set>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <map>
namespace legalizer {

struct Row {        
private:
    int row_id;
    int x,y;
    int width,height;
public:
    Row(int row_id, int x, int y, int width, int height): row_id(row_id), x(x), y(y), width(width), height(height){}
    int get_id() const {
        return row_id;
    }
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

struct Site {
private:
    int id;
    int x,y;
    int width,height;
    bool empty;
public:
    Site(int id, int x, int y, int width, int height): id(id), x(x), y(y), width(width), height(height), empty(true){}
    int get_id() const {
        return id;
    }
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
    bool is_empty() const{
        return empty;
    }
    void set_empty(bool empty){
        this->empty = empty;
    }
    void set_id(int id){
        this->id = id;
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
            int row_id = rows.size();
            rows.push_back(Row{row_id,x,y,site_width*site_num,site_height});            

            for(int i = 0; i < site_num; i++){
                int site_id = sites.size();
                sites.push_back(Site{site_id,x + i * site_width,y,site_width,site_height});                
                row_id_to_sites_id_map[row_id].push_back(site_id);
            }

            if(this->site_height == -1){
                this->site_height = site_height;
                this->site_width = site_width;
            }else{
                if(this->site_height != site_height || this->site_width != site_width){
                    std::cerr << "Site height and width should be the same for all rows" << std::endl;
                    available = false;
                }
            }
        }
        void init();
        void init_remining_sites();
        void check_init_placement_is_on_site();// ?
        bool check_on_site();
        bool legalize();
        void init_blockage();
        void remove_overflow_bins();
        void place_available_cells_on_empty_sites();
        void place_available_cells_on_empty_sites_sort_by_slack();
        void move_unavailable_cells_to_empty_sites();
        void move_unavailable_cells_to_empty_sites_sort_by_slack();
        void print_empty_sites() const;        
        int nearest_empty_site(int x, int y) const;
        std::vector<int> nearest_empty_site_enough_space(int x, int y, int rx, int ry);        
        std::vector<int> empty_sites_enough_space(int x, int y, int rx, int ry);
        bool extend_at_site_id(const int, std::vector<int> &sites_id,int x_site_num,int y_site_num);
        bool valid() const{
            return not_on_site_cells_id.empty();
        }
        std::vector<int> distance_order_empty_sites(int x, int y);
        bool try_extend_at_multiple_sites_id(const std::vector<int> &empty_sites_id,int x_site_num,int y_site_num,std::vector<int> &ret_sites_id);
        // sort
        void sort_rows_by_y();
        void sort_sites_by_y_then_x();
        // site width and height
        int get_site_width() const{
            return site_width;
        }
        int get_site_height() const{
            return site_height;
        }
        void remove_cell(int cell_id);
        void replacement_cell(int cell_id);
        void add_cell(int cell_id);
        
        void switch_to_other_solution(const std::unordered_map<int,std::vector<int>> &cell_id_to_site_id_map);
        const std::unordered_map<int,std::vector<int>>& get_cell_id_to_site_id_map() const{
            return cell_id_to_site_id_map;
        }        
        const std::vector<int> &get_cell_site_ids(int cell_id) const{
            return cell_id_to_site_id_map.at(cell_id);
        }
        void update_cell_to_site(int cell_id, const std::vector<int> &site_ids);
        bool replace_all();
        void tetris();
    private:   
        std::vector<Row> rows; // const 
        std::unordered_map<int,std::vector<int>> row_id_to_sites_id_map; //const
        
        // quick site access, check
        std::vector<Site> sites; // const        
        std::map<std::pair<int,int>,int> sites_xy_to_id_map; // const
        std::unordered_set<int> empty_sites_id; // accoring to netlist

        // site cell relation
        std::unordered_set<int> not_on_site_cells_id; // accoring to netlist
        std::unordered_map<int,int> site_id_to_cell_id_map; // accoring to netlist
        // SOLUTION:
        std::unordered_map<int,std::vector<int>> cell_id_to_site_id_map; // accoring to netlist

        int site_width, site_height;
        bool available;
};

}

#endif // LEGALIZER_H