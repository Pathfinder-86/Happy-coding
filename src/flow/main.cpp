#include <iostream>
#include "../command/command_manager.h"
#include "../config/config_manager.h"
#include <string>
// ./executable data/tiny/input.txt res/temp.txt -f runtcl
int main(int argc,char *argv[]) {
    std::cout << "Hello, 2024 ICCAD ProblemB, Let's go !" << std::endl;
    config::ConfigManager& config_manager = config::ConfigManager::get_instance();
    config_manager.parse_config(argc, argv);

    std::string input_file; 
    std::string output_file;
    if(argc > 2) {
        input_file = std::string(argv[1]);
        output_file = std::string(argv[2]);
    } else {
        std::cout << "Run tiny case" << std::endl;
        input_file = "data/tiny/input.txt";
        output_file = "res/tiny/output.txt";
    }
    command::CommandManager& command_manager = command::CommandManager::get_instance();
    command_manager.read_input_data(input_file);
    command_manager.write_output_data(output_file);
    return 0;
}