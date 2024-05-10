#ifndef CIRCUIT_CELL_H
#define CIRCUIT_CELL_H

#include <vector>
namespace circuit {
class Pin;
class Cell {
private:
    std::vector<int> pins_id;
    int x, y;
    int w, h;
    int id;
public:
    Cell(){}
    Cell(int x, int y, int w, int h) :  x(x), y(y), w(w), h(h) {}
    int get_x() const { return x; }
    int get_y() const { return y; }
    int get_w() const { return w; }
    int get_h() const { return h; }
    int get_rx() const { return x + w; }
    int get_ry() const { return y + h; }
    int get_id() const { return id; }
    void set_x(int x) { this->x = x; }
    void set_y(int y) { this->y = y; }
    void set_w(int w) { this->w = w; }
    void set_h(int h) { this->h = h; }
    void set_id(int id) { this->id = id; }
    void add_pin_id(int pin_id) { pins_id.push_back(pin_id); }
    const std::vector<int>& get_pins_id() const { return pins_id; }
};
}
#endif // CELL_H