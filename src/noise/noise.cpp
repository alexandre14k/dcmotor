#include "noise.hpp"

Noise::Noise() {}

void Noise::set_enabled(bool en) {
    enabled.store(en);
}

void Noise::set_amplitude(double amp) {
    amplitude.store(amp);
}

double Noise::generate() {
    if (!enabled.load()) return 0.0;
    double u1 = std::max(1e-6, (double)rand() / RAND_MAX);
    double u2 = (double)rand() / RAND_MAX;
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return z * amplitude.load() * 10.0;
}