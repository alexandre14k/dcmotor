#include "logger.hpp"

static GLogWriterOutput
log_writer(GLogLevelFlags log_level,
    const GLogField* fields, gsize n_fields,
    gpointer user_data) {

    if (log_level & (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL)) {
        for (gsize i = 0; i < n_fields; ++i) {
            if (g_strcmp0(fields[i].key, "MESSAGE") == 0) {
                const char* msg = (const char*)fields[i].value;
                if (msg && (g_strstr_len(msg, -1, "gtk_label_set_label") != nullptr ||
                            g_strstr_len(msg, -1, "FIXME: Implement") != nullptr ||
                            g_strstr_len(msg, -1, "gtk_widget_remove_css_class") != nullptr ||
                            g_strstr_len(msg, -1, "gtk_widget_add_css_class") != nullptr ||
                            g_strstr_len(msg, -1, "gtk_progress_bar_set_fraction") != nullptr ||
                            g_strstr_len(msg, -1, "No IM module matching") != nullptr)) {
                    return G_LOG_WRITER_HANDLED;
                }
            }
        }
    }
    return g_log_writer_default(log_level, fields, n_fields, user_data);
}

void setup_log_filter() {
    g_log_set_writer_func(log_writer, nullptr, nullptr);
}

void Logger::start() {
    running = true;
    start_time = clk::now();
    thread = std::thread(&Logger::run, this);
}

void Logger::stop() {
    running = false;
    log("__STOP__");
    if(thread.joinable()) thread.join();
}

void Logger::log(const m_string& msg) {
    if (!debug_enabled.load() && msg != "__STOP__") return;
    q.push(msg);
}

void Logger::set_debug_enabled(bool en) {
    debug_enabled.store(en);
}

long long Logger::get_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        clk::now() - start_time).count();
}

void Logger::run() {
    m_string msg;
    while(running) {
        if(q.wait_pop(msg)) {
            if(msg == "__STOP__") break;
            std::cerr << msg << std::endl;
        }
    }
}