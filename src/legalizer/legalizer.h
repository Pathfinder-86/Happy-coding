#ifndef LEGALIZER_H
#define LEGALIZER_H
#include <set>
#include <vector>
#include <iostream>
namespace legalizer {

struct Row {
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
};

struct Site {
    Site(int x, int y, int width, int height): x(x), y(y), width(width), height(height){}
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
};

class Legalizer{
    public:
        Legalizer(){}
        static Legalizer& get_instance() {
            static Legalizer instance;        
            return instance;
        }
        void add_row(int x, int y, int site_width, int site_height, int site_num){
            rows.push_back(Row{x,y,site_width*site_num,site_height});
            for(int i = 0; i < site_num; i++){
                sites.push_back(Site(x + i*site_width, y, site_width, site_height));
                //std::cout << "Site " << x + i*site_width << "," << y << std::endl;
                sites_set.insert(std::make_pair(x + i*site_width, y));
            }
        }
        void init(){
            init_rows = rows;
            init_sites = sites;
        }
        void reset(){
            rows = init_rows;
            sites = init_sites;
        }
        bool check_on_site();
    private:   
        std::vector<Row> rows;
        std::vector<Site> sites;
        std::vector<Row> init_rows;
        std::vector<Site> init_sites;
        std::set<std::pair<int,int>> sites_set;
};

}

#endif // LEGALIZER_H