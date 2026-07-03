#pragma once
#include "main.hpp"

struct PlotPoint {
    double t;
    double pwm;
    double rpm;
    double Kp;
    double Ki;
    double Kd;
};

struct PlotColors {
    double bg_r, bg_g, bg_b;
    double grid_r, grid_g, grid_b;
    double text_r, text_g, text_b;
    double pwm_r, pwm_g, pwm_b;
    double rpm_r, rpm_g, rpm_b;
    double kp_r, kp_g, kp_b;
    double ki_r, ki_g, ki_b;
    double kd_r, kd_g, kd_b;
};

struct MotorPlot {
public:
    MotorPlot();
    ~MotorPlot();
    GtkWidget* get_widget();
    void add_point(double t, double pwm, double rpm,
                   double Kp, double Ki, double Kd);
    void set_log_view(bool en);
    void set_tracking(bool en, double rpm);
    void adjust_time_window(double delta);
private:
    GtkWidget* area = nullptr;
    std::atomic<bool> log_view{false};
    std::atomic<bool> track_enabled{false};
    std::atomic<double> track_rpm{0.0};
    std::atomic<double> time_window{10.0};
    std::vector<PlotPoint> data;

    void ensure_init();
    static void on_draw(GtkDrawingArea* area, cairo_t* cr,
        int width, int height, gpointer data);
    static void draw_background_and_grid(cairo_t* cr, int width,
        int height, double pad_l, double pad_r, double pad_t,
        double pad_b, bool log_view, const PlotColors& colors);
    static void draw_title_and_legend(cairo_t* cr, double pad_l,
        double plot_w, double pad_t, bool log_view, const PlotColors& colors, bool track_en);
    static void draw_axis_labels(cairo_t* cr, int height, double pad_l,
        double plot_w, double pad_t, double pad_b, bool log_view, double max_pid, const PlotColors& colors);
    static void draw_plot_line(cairo_t* cr, const std::vector<PlotPoint>& data,
        double min_t, double range_t, double plot_w, double plot_h,
        double pad_l, double pad_t,
        std::function<double(const PlotPoint&)> getter,
        double max_val, double r, double g, double b, bool log_view);
};