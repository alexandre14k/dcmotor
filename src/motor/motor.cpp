#include "motor.hpp"

Motor::Motor(Plant& p, Logger& l) : plant(p), logger(l) {}
Motor::~Motor() { stop(); }

void Motor::start() {
    running = true;
    thread = std::thread(&Motor::run, this);
}

void Motor::stop() {
    running = false;
    GuiToMotor stop_msg;
    motor_q.push(stop_msg);
    if(thread.joinable()) thread.join();
}

void Motor::set_target(double rpm) {
    target_rpm = rpm;
}

void Motor::set_params(MotorParams p) {
    params = p;
}

BlockingQueue<MotorToGui>& Motor::get_gui_q() {
    return gui_q;
}

BlockingQueue<GuiToMotor>& Motor::get_motor_q() {
    return motor_q;
}

void Motor::process_message(GuiToMotor& msg) {
    if(msg.type == GuiToMotor::SET_TARGET) {
        target_rpm = msg.target_rpm;
        logger.log(fmt::format("{},MOTOR,target,{:.2f}", logger.get_ms(), msg.target_rpm));
    } else if(msg.type == GuiToMotor::SET_PID_PARAMS) {
        params = msg.pid_params;
        logger.log(fmt::format("{},MOTOR,pid,{:.2f},{:.2f},{:.2f}", logger.get_ms(), params.Kp, params.Ki, params.Kd));
    } else if(msg.type == GuiToMotor::SET_PLANT_PARAMS) {
        plant.set_params(msg.plant_params);
        logger.log(fmt::format("{},MOTOR,plant,{:.2f}", logger.get_ms(), msg.plant_params.R));
    } else if(msg.type == GuiToMotor::SET_NOISE_PARAMS) {
        plant.set_noise(msg.noise_enabled, msg.noise_amp);
        logger.log(fmt::format("{},MOTOR,noise,{},{:.4f}", logger.get_ms(), msg.noise_enabled, msg.noise_amp));
    } else if(msg.type == GuiToMotor::SET_TRACK_PARAMS) {
        track_enabled.store(msg.track_enabled);
        track_rpm.store(msg.track_rpm);
        logger.log(fmt::format("{},MOTOR,track,{},{:.2f}", logger.get_ms(), msg.track_enabled, msg.track_rpm));
    } else if(msg.type == GuiToMotor::SET_PWM_DISTURBANCE) {
        pwm_disturbance.store(msg.pwm_disturbance);
        logger.log(fmt::format("{},MOTOR,disturb_pwm,{:.2f}", logger.get_ms(), msg.pwm_disturbance));
    } else if(msg.type == GuiToMotor::SET_RPM_DISTURBANCE) {
        plant.set_load_disturbance(msg.rpm_disturbance);
        logger.log(fmt::format("{},MOTOR,disturb_rpm,{:.2f}", logger.get_ms(), msg.rpm_disturbance));
    } else if(msg.type == GuiToMotor::SET_PAUSE) {
        plant.set_paused(msg.paused);
        logger.log(fmt::format("{},MOTOR,pause,{}", logger.get_ms(), msg.paused));
    }
}

void Motor::process_plant_state(PlantToMotor& pmsg) {
    double current_rpm = pmsg.omega * 60.0 / (2.0 * M_PI);
    double dt = 0.01;
    double final_voltage = 0.0;
    double pwm_out = 0.0;

    if (track_enabled.load()) {
        double target_w = track_rpm.load() * 2.0 * M_PI / 60.0;
        double error = target_w - pmsg.omega;

        integral += error * dt;
        if(integral > 12.0) integral = 12.0;
        if(integral < 0.0) integral = 0.0;

        double deriv = (error - prev_error) / dt;
        double alpha = 0.15;
        filtered_deriv = alpha * deriv + (1.0 - alpha) * filtered_deriv;
        prev_error = error;

        double pid = params.Kp * error +
                     params.Ki * integral +
                     params.Kd * filtered_deriv;

        if(pid > 12.0) pid = 12.0;
        else if(pid < 0.0) pid = 0.0;

        final_voltage = pid + pwm_disturbance.load();
    } else {
        integral = 0.0;
        prev_error = 0.0;
        filtered_deriv = 0.0;
        final_voltage = (target_rpm.load() / 100.0) * 12.0 +
                        pwm_disturbance.load();
    }

    if(final_voltage > 12.0) final_voltage = 12.0;
    if(final_voltage < 0.0) final_voltage = 0.0;

    pwm_out = final_voltage / 12.0;
    plant.set_voltage(final_voltage);

    int state = 0;
    if (track_enabled.load()) {
        double error_rpm = std::abs(track_rpm.load() - current_rpm);
        if (std::isnan(current_rpm) || std::isnan(track_rpm.load()) ||
            error_rpm > 1000.0) {
            state = 2;
        } else if (error_rpm > 100.0) {
            state = 1;
        }
    } else {
        if (std::isnan(current_rpm)) {
            state = 2;
        } else if (pwm_out >= 0.999 && (target_rpm.load() / 100.0) >= 0.999) {
            state = 2;
        }
    }

    if(std::abs(current_rpm - last_logged_rpm) > 1.0 ||
       std::abs(pwm_out*100 - last_logged_pwm) > 1.0) {
        logger.log(fmt::format("{},MOTOR,state,{:.0f},{:.0f},{}",
            logger.get_ms(), current_rpm, pwm_out*100, state));
        last_logged_rpm = current_rpm;
        last_logged_pwm = pwm_out*100;
    }

    MotorToGui gmsg{current_rpm, pwm_out * 100.0, params.Kp, params.Ki,
                    params.Kd, state};
    gui_q.push(gmsg);
}

void Motor::run() {
    while(running) {
        // Highest priority for user commands
        GuiToMotor msg;
        while(motor_q.pop(msg)) {
            process_message(msg);
        }

        PlantToMotor pmsg;
        while(plant.get_queue().pop(pmsg)) {
            process_plant_state(pmsg);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}