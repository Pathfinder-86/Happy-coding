#include "command_manager.h"
#include "../design/design.h"
namespace command {
    void CommandManager::run() {
        design::Design &design = design::Design::get_instance();
        design.update_bins_utilization();
    }
}