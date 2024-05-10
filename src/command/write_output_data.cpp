#include "command_manager.h"
#include <string>
#include <iostream>
namespace command{
void CommandManager::write_output_data(const std::string &filename) {
    // Function implementation goes here
    std::cout<<"write data to output"<<filename<<std::endl;
}
}