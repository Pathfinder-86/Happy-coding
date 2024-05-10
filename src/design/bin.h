#ifndef BIN_H
#define BIN_H

namespace design {
class Bin {
    public:
        Bin() { utilization = 0.0; }
        Bin(double x, double y, double w, double h) : x(x), y(y), w(w), h(h) { utilization = 0.0;}
        double get_x() const { return x; }
        double get_y() const { return y; }
        double get_w() const { return w; }
        double get_h() const { return h; }
        double get_rx() const { return x + w; }
        double get_ry() const { return y + h; }
        double get_utilization() const { return utilization; }
        void set_x(double x) { this->x = x; }
        void set_y(double y) { this->y = y; }
        void set_w(double w) { this->w = w; }
        void set_h(double h) { this->h = h; }
        void set_utilization(double utilization) { this->utilization = utilization; }
    private:
        double x, y;
        double w, h;
        double utilization;
};
}

#endif // BIN_H