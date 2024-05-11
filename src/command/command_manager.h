#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <string>

namespace command {

    class CommandManager {
    public:
        static CommandManager& get_instance() {
            static CommandManager instance;
            return instance;
        }

    
    public:
        void run(){};
        // Read
        void read_input_data(const std::string& filename);        
        // Write
        void write_output_data(const std::string& filename);
        void write_output_layout_data(const std::string& filename);
        void write_output_utilization_data(const std::string& filename);
    private:
        CommandManager() {}
        CommandManager(const CommandManager&) = delete;
        CommandManager& operator=(const CommandManager&) = delete;
    };

} // namespace command

#endif // COMMAND_MANAGER_H