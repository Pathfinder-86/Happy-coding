#ifndef LIBCELL_H
#define LIBCELL_H

#include <string>
#include <vector>
#include <iostream>

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
        void add_pin(const std::string& pin_name, double x, double y,int pin_type) {            
            if(pin_type == 0){ // input
                input_pins_name.push_back(pin_name);
                input_pins_position.push_back(std::make_pair(x, y));
            }
            else if(pin_type == 1){ // output
                output_pins_name.push_back(pin_name);
                output_pins_position.push_back(std::make_pair(x, y));            
            }else{ // clk (other)
                other_pins_name.push_back(pin_name);
                other_pins_position.push_back(std::make_pair(x, y));            
            }
        }

        std::vector<std::string> get_pins_name() const {                     
            std::vector<std::string> all_pins_name;
            all_pins_name.insert(all_pins_name.end(), input_pins_name.begin(), input_pins_name.end());
            all_pins_name.insert(all_pins_name.end(), output_pins_name.begin(), output_pins_name.end());
            all_pins_name.insert(all_pins_name.end(), other_pins_name.begin(), other_pins_name.end());
            //for(auto pin_name : all_pins_name){
            //    std::cout << pin_name << std::endl;
            //}
            return all_pins_name;            
        }

        const std::vector<std::string>& get_input_pins_name() const { return input_pins_name; }
        const std::vector<std::string>& get_output_pins_name() const { return output_pins_name; }
        const std::vector<std::string>& get_other_pins_name() const { return other_pins_name; }

        const std::vector<std::pair<double, double>>& get_input_pins_position() const { return input_pins_position; }        
        const std::vector<std::pair<double, double>>& get_output_pins_position() const { return output_pins_position; }
        const std::vector<std::pair<double, double>>& get_other_pins_position() const { return other_pins_position; }
        std::vector<std::pair<double, double>> get_pins_position() const {
            std::vector<std::pair<double, double>> all_pins_postion;
            all_pins_postion.insert(all_pins_postion.end(), input_pins_position.begin(), input_pins_position.end());
            all_pins_postion.insert(all_pins_postion.end(), output_pins_position.begin(), output_pins_position.end());
            all_pins_postion.insert(all_pins_postion.end(), other_pins_position.begin(), other_pins_position.end());

            //for(auto pin_position : all_pins_postion){
            //    std::cout << pin_position.first << " " << pin_position.second << std::endl;
            //}
            return all_pins_postion;
        }

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
        void set_bits(int bits) { this->bits = bits; }
        int get_bits() const { return bits; }
        void set_sequential(bool is_sequential) { this->is_sequential = is_sequential; }
        bool get_sequential() const { return is_sequential; }
        void anaylze_input_data();
    private:
        bool is_sequential;
        int bits = 0;
        int id;
        std::string name;
        double width;
        double height;
        std::vector<std::string> input_pins_name;
        std::vector<std::string> output_pins_name;
        std::vector<std::string> other_pins_name;
        std::vector<std::pair<double, double>> input_pins_position;
        std::vector<std::pair<double, double>> output_pins_position;
        std::vector<std::pair<double, double>> other_pins_position;
        double power;
        double area;
        double delay = 0.0;

    };
}

#endif // LIBCELL_H