#include "command_manager.h"
#include <iostream>
#include "../runtime/runtime.h"
namespace command{

void CommandManager::dry_banking(){
    const runtime::RuntimeManager& runtime = runtime::RuntimeManager::get_instance();
    runtime.get_runtime();
    std::cout << "INIT dry_banking" << std::endl;    
    
    



    runtime.get_runtime();
    std::cout << "Finish dry_banking" << std::endl;    
}


}