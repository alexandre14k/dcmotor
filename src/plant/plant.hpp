#pragma once
#include "main.hpp"
#include "debug/logger.hpp"
#include "noise/noise.hpp"

struct PlantParams {
    double J = 0.001;
    double B = 0.00001;
    double Kt = 0.02;
    double Ke = 0.02;
    double R = 1.0;
};

struct PlantState {
    double omega = 0.0;
    double theta = 0.0;
};

struct PlantToMotor {
    double omega;
    double theta;
};

struct Plant {
public:
    Plant(Logger& l);
    ~Plant();
    void start();
    void stop();
    void set_params(PlantParams p);
    void set_voltage(double v);
    void set_noise(bool enabled, double amp);
    void set_load_disturbance(double val);
    void set_paused(bool en);
    BlockingQueue<PlantToMotor>& get_queue();
private:
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::thread thread;
    Logger& logger;
    PlantParams params;
    std::atomic<double> voltage{0.0};
    std::atomic<double> load_disturbance{0.0};
    PlantState state;
    Noise noise;
    BlockingQueue<PlantToMotor> out_q;
    double last_logged_omega = -1.0;

    void run();
    void calculate_dynamics(double dt);
    void apply_noise_and_notify();
};