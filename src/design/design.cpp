#include "design.h"
#include <iostream>

namespace design {
    
void Design::init_bins(){
    double bin_width =  get_bin_width();
    double bin_height = get_bin_height();
    int col = get_die_width() / bin_width;
    int row = get_die_height() / bin_height;
    std::cout << "Die width: " << get_die_width() << " Die height: " << get_die_height() << std::endl;
    std::cout << "Init bins row: " << row <<" col: " << col << std::endl;
    bins.resize(row);
    for(int i = 0; i < row; i++){
        for(int j = 0; j < col; j++){
            bins.at(i).push_back(Bin(j * bin_width, i * bin_height, bin_width, bin_height));            
        }
    }
}

}