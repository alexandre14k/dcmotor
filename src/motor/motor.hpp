#pragma once
#include "main.hpp"
#include "plant/plant.hpp"
#include "debug/logger.hpp"

struct MotorParams {
    double Kp = 0.1;
    double Ki = 1.0;
    double Kd = 0.001;
};

struct MotorToGui {
    double rpm;
    double pwm;
    double Kp;
    double Ki;
    double Kd;
    int state;
};

struct GuiToMotor {
    enum Type {
        SET_TARGET,
        SET_PID_PARAMS,
        SET_PLANT_PARAMS,
        SET_NOISE_PARAMS,
        SET_TRACK_PARAMS,
        SET_PWM_DISTURBANCE,
        SET_RPM_DISTURBANCE,
        SET_PAUSE
    } type;
    double target_rpm;
    MotorParams pid_params;
    PlantParams plant_params;
    bool noise_enabled;
    double noise_amp;
    bool track_enabled;
    double track_rpm;
    double pwm_disturbance;
    double rpm_disturbance;
    bool paused;
};

struct Motor {
public:
    Motor(Plant& p, Logger& l);
    ~Motor();
    void start();
    void stop();
    void set_target(double rpm);
    void set_params(MotorParams p);
    BlockingQueue<MotorToGui>& get_gui_q();
    BlockingQueue<GuiToMotor>& get_motor_q();
private:
    std::atomic<bool> running{false};
    std::thread thread;
    Plant& plant;
    Logger& logger;
    MotorParams params;
    std::atomic<double> target_rpm{0.0};
    std::atomic<bool> track_enabled{false};
    std::atomic<double> track_rpm{0.0};
    std::atomic<double> pwm_disturbance{0.0};
    double integral{0.0};
    double prev_error{0.0};
    double filtered_deriv{0.0};

    BlockingQueue<MotorToGui> gui_q;
    BlockingQueue<GuiToMotor> motor_q;

    double last_logged_rpm = -1.0;
    double last_logged_pwm = -1.0;

    void run();
    void process_message(GuiToMotor& msg);
    void process_plant_state(PlantToMotor& pmsg);
};