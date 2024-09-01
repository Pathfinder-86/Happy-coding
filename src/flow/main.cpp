#include <iostream>
#include "../command/command_manager.h"
#include "../config/config_manager.h"
#include <string>
#include <../runtime/runtime.h>
// ./executable data/testcase/input.txt res/output.txt -f runtcl
// ./executable data/tiny/input.txt res/output.txt -f runtcl
int main(int argc,char *argv[]) {

    const runtime::RuntimeManager& runtime_manager = runtime::RuntimeManager::get_instance();
    
    std::cout << "Hello, 2024 ICCAD ProblemB, Let's go !" << std::endl;
    config::ConfigManager& config_manager = config::ConfigManager::get_instance();
    config_manager.parse_config(argc, argv);

    std::string input_file; 
    std::string output_file;
    if(argc > 2) {
        input_file = std::string(argv[1]);
        output_file = std::string(argv[2]);
    } else {
        std::cerr << "Usage: ./executable input_file output_file" << std::endl;
        return 1;
    }

    command::CommandManager& command_manager = command::CommandManager::get_instance();
    if(std::get<bool>(config_manager.get_config_value("testing_mode"))) {
        command_manager.read_input_data(input_file);        
        //command_manager.test_cluster_ff();
        //command_manager.test_decluster_ff();
        //command_manager.SA();
        //command_manager.kmeans_cluster();
        //command_manager.swap_ff();
        //command_manager.write_output_data(output_file);
        if(std::get<bool>(config_manager.get_config_value("run_tiny"))) {
            command_manager.write_output_data_from_best_solution(output_file);
            command_manager.write_output_layout_data("plot/tiny/layout.txt");
            command_manager.write_output_utilization_data("plot/tiny/utilization.txt");
        }else{
            command_manager.write_output_data_from_best_solution(output_file);
            command_manager.write_output_layout_data("plot/testcase/layout.txt");
            command_manager.write_output_utilization_data("plot/testcase/utilization.txt");
        }
        runtime_manager.get_runtime();
    }else{
        // Contest mode
        command_manager.read_input_data(input_file);        
        //command_manager.SA();      
        command_manager.kmeans_cluster();
        //command_manager.swap_ff();  
        command_manager.write_output_data_from_best_solution(output_file);
        runtime_manager.get_runtime();
    }

    

    return 0;
}