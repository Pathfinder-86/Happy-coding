#ifndef LIBCELL_H
#define LIBCELL_H

#include <string>
#include <vector>

namespace design {
    class LibCell {
        
    public:
        LibCell() {}
        LibCell(const std::string& name, double width, double height) : name(name), width(width), height(height) {
            power = 0.0;
            area = width * height;
            delay = 0.0;
        }
        const std::string& get_name() const { return name; }
        double get_width() const { return width; }
        double get_height() const { return height; }
        void add_pin(const std::string& pin_name, double x, double y) {
            pins_name.push_back(pin_name);
            pins_position.push_back(std::make_pair(x, y));
        }
        const std::vector<std::string>& get_pins_name() const { return pins_name; }
        const std::vector<std::pair<double, double>>& get_pins_position() const { return pins_position; }        
        void set_power(double power) { this->power = power; }
        double get_power() const { return power; }
        double get_area() const { return area; }
        void set_delay(double delay) { this->delay = delay; }
        double get_delay() const { return delay; }
        double get_cost(double power_factor, double area_factor, double delay_factor) const{
            return power * power_factor + area * area_factor + delay * delay_factor;
        }
        void set_id(int id) { this->id = id; }
        int get_id() const { return id; }
        void set_sequential(bool is_sequential) { this->is_sequential = is_sequential; }
        bool get_sequential() const { return is_sequential; }
    private:
        bool is_sequential;
        int id;
        std::string name;
        double width;
        double height;
        std::vector<std::string> pins_name;
        std::vector<std::pair<double, double>> pins_position;
        double power;
        double area;
        double delay;

    };
}

#endif // LIBCELL_H