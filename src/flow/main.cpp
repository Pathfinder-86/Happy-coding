#include <iostream>
#include "../command/command_manager.h"
#include "../config/config_manager.h"
#include <string>
// ./executable data/testcase/input.txt res/output.txt -f runtcl
// ./executable data/tiny/input.txt res/output.txt -f runtcl
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
        std::cout << "Run testcase" << std::endl;
        input_file = "data/testcase/input.txt";
        output_file = "res/testcase/output.txt";
    }
    command::CommandManager& command_manager = command::CommandManager::get_instance();
    if(std::get<bool>(config_manager.get_config_value("runtcl_mode"))) {
        command_manager.read_input_data(input_file);
        command_manager.run();
        command_manager.test_cluster_ff();
        command_manager.write_output_data(output_file); 
        command_manager.write_output_data_from_best_solution("res/testcase/output_best.txt");
        if(std::get<bool>(config_manager.get_config_value("plot_mode"))) {                                    
            command_manager.write_output_layout_data("plot/testcase/layout.txt");
            command_manager.write_output_utilization_data("plot/testcase/utilization.txt");
        }
    }else{
        command_manager.read_input_data(input_file);
        command_manager.run();
        command_manager.write_output_data(output_file);        
        if(std::get<bool>(config_manager.get_config_value("plot_mode"))) {                                    
            command_manager.write_output_layout_data("plot/testcase/layout.txt");
            command_manager.write_output_utilization_data("plot/testcase/utilization.txt");
        }
    }

    

    return 0;
}