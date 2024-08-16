#include <chrono>
#include <iostream>
namespace runtime {
    class RuntimeManager {
    private:
        std::chrono::steady_clock::time_point start_time;        
        RuntimeManager() {
            start_time = std::chrono::steady_clock::now();
        }

    public:
        // Get the singleton instance
        static RuntimeManager &get_instance() {
            static RuntimeManager instance;
            return instance;
        }

        void get_runtime() const {
            std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed_seconds = current_time - start_time;
            double runtime_seconds = elapsed_seconds.count();
            int hours = runtime_seconds / 3600;
            int minutes = (runtime_seconds / 60) - (hours * 60);
            int seconds = runtime_seconds - (hours * 3600) - (minutes * 60);
            std::cout << "Runtime: " << hours << " h/ " << minutes << " min/ " << seconds << " sec" << std::endl;
        }
    };
}