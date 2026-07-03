#pragma once
#include "main.hpp"

struct Logger {
public:
    void start();
    void stop();
    void log(const m_string& msg);
    void set_debug_enabled(bool en);
    long long get_ms();
private:
    std::atomic<bool> running{false};
    std::atomic<bool> debug_enabled{false};
    std::thread thread;
    BlockingQueue<m_string> q;
    clk::time_point start_time;
    void run();
};

void setup_log_filter();