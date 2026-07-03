#pragma once
#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <functional>
#include <gtk/gtk.h>
#include <glib.h>
#include <fmt/core.h>

using m_string = std::string;
static auto& m_cout = std::cout;
inline auto m_endl = std::endl<char, std::char_traits<char>>;

using clk = std::chrono::steady_clock;
using sec = std::chrono::duration<double>;

constexpr auto APP_TITLE = "DC Motor Emulator";
constexpr auto APP_URL = "https://github.com/alexandre14k/dcmotor/";

constexpr int SLOW_MODE_MS = 500;
constexpr int NORMAL_MODE_MS = 250;
constexpr int FAST_MODE_MS = 100;

template<typename T>
struct BlockingQueue {
public:
    void push(T const& v) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            q.push(v);
        }
        cv.notify_one();
    }
    bool pop(T& v) {
        std::unique_lock<std::mutex> lk(mtx);
        if(q.empty()) return false;
        v = q.front();
        q.pop();
        return true;
    }
    bool wait_pop(T& v) {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this]{return !q.empty();});
        v = q.front();
        q.pop();
        return true;
    }
    void clear() {
        std::lock_guard<std::mutex> lk(mtx);
        while(!q.empty()) q.pop();
    }
private:
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
};