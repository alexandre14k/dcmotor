#include "plant.hpp"

Plant::Plant(Logger& l) : logger(l) {}
Plant::~Plant() { stop(); }

void Plant::start() {
    running = true;
    thread = std::thread(&Plant::run, this);
}

void Plant::stop() {
    running = false;
    if(thread.joinable()) thread.join();
}

void Plant::set_params(PlantParams p) {
    params = p;
}

void Plant::set_voltage(double v) {
    voltage = v;
}

void Plant::set_noise(bool enabled, double amp) {
    noise.set_enabled(enabled);
    noise.set_amplitude(amp);
}

void Plant::set_load_disturbance(double val) {
    load_disturbance.store(val);
}

void Plant::set_paused(bool en) {
    paused.store(en);
}

BlockingQueue<PlantToMotor>& Plant::get_queue() {
    return out_q;
}

void Plant::calculate_dynamics(double dt) {
    double V = voltage.load();
    double w = state.omega;

    double I = (V - params.Ke * w) / params.R;
    double T = params.Kt * I - load_disturbance.load();
    double dw = (T - params.B * w) / params.J;

    state.omega += dw * dt;
    state.theta += state.omega * dt;

    if(state.omega < 0) state.omega = 0;
}

void Plant::apply_noise_and_notify() {
    double out_omega = state.omega + noise.generate();

    if(std::abs(out_omega - last_logged_omega) > 0.1) {
        logger.log(fmt::format("{},PLANT,omega,{:.2f}", logger.get_ms(), out_omega));
        last_logged_omega = out_omega;
    }

    PlantToMotor msg{out_omega, state.theta};
    out_q.push(msg);
}

void Plant::run() {
    auto last = clk::now();
    while(running) {
        if (paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            last = clk::now();
            continue;
        }

        auto now = clk::now();
        double dt = sec(now - last).count();
        last = now;

        calculate_dynamics(dt);
        apply_noise_and_notify();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}