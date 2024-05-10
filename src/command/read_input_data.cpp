#include "command_manager.h"
#include <string>
#include <iostream>
#include "../circuit/circuit.h"
namespace command{
void CommandManager::read_input_data(const std::string &filename) {    
    std::cout<<"read data from input"<<filename<<std::endl;
    circuit::Netlist &netlist = circuit::Netlist::get_instance();
}
}