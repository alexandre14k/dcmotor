#pragma once
#include "main.hpp"

struct Noise {
public:
    Noise();
    void set_enabled(bool en);
    void set_amplitude(double amp);
    double generate();
private:
    std::atomic<bool> enabled{false};
    std::atomic<double> amplitude{0.0};
};