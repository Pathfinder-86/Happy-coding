#ifndef UTILIZATION_H
#define UTILIZATION_H
#include "../design/bin.h"
#include <vector>
namespace legalizer {
class UtilizationCalculator {
    public:        
        UtilizationCalculator(){
            init_bins();
        }
        static UtilizationCalculator& get_instance() {
            static UtilizationCalculator instance;
            return instance;
        }
        void init_bins();
        void update_bins_utilization();
        void reset_bins_utilization();
        int get_overflow_bins() const {
            return overflow_bins;
        }
        const std::vector<std::vector<design::Bin>>& get_bins() const {
            return bins;
        }
    private:
        std::vector<std::vector<design::Bin>> bins;
        int overflow_bins;
};
}
#endif // UTILIZATION_H