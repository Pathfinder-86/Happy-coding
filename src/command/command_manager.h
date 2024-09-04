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
        void run();
        // Read
        void read_input_data(const std::string& filename);        
        // Write
        void write_output_data(const std::string& filename);
        void write_output_data_from_best_solution(const std::string& filename);
        void write_output_layout_data(const std::string& filename);
        void write_output_utilization_data(const std::string& filename);
        // function
        void test_cluster_ff();
        void test_decluster_ff();
        void SA();
        void kmeans_cluster();
        void faster_clustering();
        bool extremely_fast_clustering();        
        void clustering_clean_solution();
        bool extremely_fast_clustering_limit_high_bits();
        void swap_ff();
        bool clustering_all_noncritical_q_pins_cells();        
        bool clustering_exist_noncritical_q_pins_cells_limit_high_bits();
        // best so far
        bool clustering_exist_noncritical_q_pins_cells();
        bool clustering_exist_noncritical_q_pins_cells_and_legal_by_timing();

        bool iterate_clustering_exist_noncritical_q_pins_cells();        
        void dry_banking();
    private:
        CommandManager() {}
        CommandManager(const CommandManager&) = delete;
        CommandManager& operator=(const CommandManager&) = delete;
    };

} // namespace command

#endif // COMMAND_MANAGER_H