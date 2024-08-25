#include "config_manager.h"
#include <iostream>
#include <string>

namespace config{

void ConfigManager::set_config(const std::string& key, const ConfigValue& new_value){    
    if(!config_map.count(key)){
        return;
    }
    Config &config = config_map[key];
    config.value = new_value;    
}

void ConfigManager::parse_config(int argc, char *argv[]){
    std::string filename;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-f") {
            if (i + 1 < argc) { 
                filename = argv[++i]; 
            } else {
                std::cerr << "-f option requires one argument." << std::endl;
                return;
            }  
        }
    }

    if(filename.empty()){
        std::cerr<<"No config file"<<std::endl;
        return;
    }

    std::ifstream input_file(filename);
    if(!input_file){
        std::cerr<<"Can't open file:"<<filename<<std::endl;
        return;
    }

    std::string line;
    while (std::getline(input_file, line)) {
        std::istringstream iss(line);
        std::string key;
        std::string value;
        if (!(iss >> key >> value)) { 
            continue; 
        }
        
        if(!config_map.count(key)){
            std::cout<<"No key:"<<key<<" in config_manager"<<std::endl;
            continue;
        }
        ConfigType type = config_map[key].type;
        if(type == ConfigType::String){            
            set_config(key,value);
        }else if(type == ConfigType::Bool){
            set_config(key, (value == "true") );
        }else if(type == ConfigType::Int){
            set_config(key, std::stoi(value) );            
        }else if(type == ConfigType::Double){
            set_config(key, std::stod(value) );
        }
    }
    input_file.close();
    
    if(std::get<bool>(get_config_value("dump_tcl_setting"))){
        std::cout<<"Dump config setting"<<std::endl;
        for(const auto &it : config_map){
            if(it.second.type == ConfigType::String){
                std::cout<<it.first<<":"<<std::get<std::string>(it.second.value)<<std::endl;
            }else if(it.second.type == ConfigType::Bool){
                std::cout<<it.first<<":"<<std::get<bool>(it.second.value)<<std::endl;
            }else if(it.second.type == ConfigType::Int){
                std::cout<<it.first<<":"<<std::get<int>(it.second.value)<<std::endl;
            }else if(it.second.type == ConfigType::Double){
                std::cout<<it.first<<":"<<std::get<double>(it.second.value)<<std::endl;
            }
        }
    }
}

ConfigValue ConfigManager::get_config_value(const std::string& key) const{    
    return config_map.at(key).value;  
}

void ConfigManager::init_config_value(){
    std::cout<<"init_config_value"<<std::endl;
    add_config("dump_tcl_setting", ConfigType::Bool, false );
    add_config("check_input_data", ConfigType::Bool, false );    
    add_config("contest_mode", ConfigType::Bool,true);
    add_config("plot_mode", ConfigType::Bool, false);
    add_config("testing_mode", ConfigType::Bool, false);
    add_config("run_tiny", ConfigType::Bool, false);
    add_config("clustering_rate", ConfigType::Double, 1.0);
    add_config("declustering_rate", ConfigType::Double, 0.0);
    add_config("max_iterations", ConfigType::Int, 10000);
    add_config("cooling_rate", ConfigType::Double, 0.95);
    add_config("time_out", ConfigType::Int, 50);
    add_config("fast_timer", ConfigType::Bool, false);
    add_config("timer_case",ConfigType::Int, 0);
}

void ConfigManager::add_config(const std::string &key, ConfigType type, const ConfigValue &value){
    Config config;
    config.type = type;
    config.value = value;
    config_map[key] = config;
}


}