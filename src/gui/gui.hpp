// src/gui/gui.hpp
#pragma once
#include "main.hpp"
#include "motor/motor.hpp"
#include "debug/logger.hpp"
#include "gui/plot.hpp"

struct GuiApp {
public:
    GuiApp(Motor& m, Logger& l);
    ~GuiApp();
    int run(int argc, char** argv);
private:
    GtkApplication* app;
    GtkWidget* window;
    GtkCssProvider* provider;
    GtkWidget* main_box;
    GtkWidget* left_box;
    GtkWidget* right_box;
    GtkWidget* plot_btn_box;
    GtkWidget* fullscreen_btn;
    GtkWidget* track_btn_overlay;
    GtkWidget* screenshot_btn_overlay;

    GtkWidget* pwm_label;
    GtkWidget* slider;
    GtkWidget* rpm_bar;
    GtkWidget* rpm_label;
    GtkWidget* pause_btn;
    GtkWidget* debug_btn;
    GtkWidget* speed_slider;
    GtkWidget* status_label;

    GtkWidget* motor_type_combo;

    GtkWidget* kp_entry;
    GtkWidget* ki_entry;
    GtkWidget* kd_entry;

    GtkWidget* r_entry;
    GtkWidget* j_entry;
    GtkWidget* b_entry;
    GtkWidget* kt_entry;
    GtkWidget* ke_entry;

    GtkWidget* pid_step_entry;
    GtkWidget* motor_step_entry;

    GtkWidget* noise_btn;
    GtkWidget* noise_amp_entry;
    GtkWidget* log_btn;

    GtkWidget* track_entry;
    GtkWidget* track_btn;
    GtkWidget* track_step_up_btn;
    GtkWidget* track_step_down_btn;

    GtkWidget* state_box;
    GtkWidget* state_label;

    GtkWidget* time_expand_btn;
    GtkWidget* time_compress_btn;

    MotorPlot plot;

    Motor& motor;
    Logger& logger;
    std::atomic<double> current_rpm{0.0};
    int last_slider_val = -1;

    guint pwm_disturb_timeout_id{0};
    guint rpm_disturb_timeout_id{0};
    guint ui_timeout_id{0};

    static void on_activate(GtkApplication* app, gpointer data);
    static void on_slider_changed(GtkRange* range, gpointer data);
    static gboolean on_update_ui(gpointer data);
    static void on_param_changed(GtkEditable* edit, gpointer data);
    static void on_param_inc(GtkButton* btn, gpointer data);
    static void on_param_dec(GtkButton* btn, gpointer data);
    static void on_param_reset(GtkButton* btn, gpointer data);
    static void on_motor_type_changed(GObject* obj, GParamSpec* pspec, gpointer data);
    static void on_noise_toggled(GtkToggleButton* btn, gpointer data);
    static void on_log_toggled(GtkToggleButton* btn, gpointer data);
    static void on_track_toggled(GtkToggleButton* btn, gpointer data);
    static void on_track_toggled_overlay(GtkToggleButton* btn, gpointer data);
    static void on_track_changed(GtkEditable* edit, gpointer data);
    static void on_track_step_up_clicked(GtkButton* btn, gpointer data);
    static void on_track_step_down_clicked(GtkButton* btn, gpointer data);
    static void on_pause_toggled(GtkToggleButton* btn, gpointer data);
    static void on_debug_toggled(GtkToggleButton* btn, gpointer data);
    static void on_fullscreen_toggled(GtkToggleButton* btn, gpointer data);
    static void on_screenshot_toggled_overlay(GtkToggleButton* btn, gpointer data);
    static void on_speed_changed(GtkRange* range, gpointer data);
    static gboolean on_speed_change_value(GtkRange* range, GtkScrollType scroll, gdouble value, gpointer data);
    static void on_screenshot_clicked(GtkButton* btn, gpointer data);
    static void on_time_expand_clicked(GtkButton* btn, gpointer data);
    static void on_time_compress_clicked(GtkButton* btn, gpointer data);

    static void on_disturb_pwm_clicked(GtkButton* btn, gpointer data);
    static void on_disturb_rpm_clicked(GtkButton* btn, gpointer data);
    static gboolean reset_pwm_disturbance(gpointer data);
    static gboolean reset_rpm_disturbance(gpointer data);

    static void on_url_clicked(GtkButton* btn, gpointer data);

    static void setup_css_provider(GuiApp* self);
    static GtkWidget* create_main_layout(GuiApp* self);
    static GtkWidget* create_left_panel(GuiApp* self);
    static void add_pid_params(GuiApp* self, GtkGrid* grid, int& row);
    static void add_motor_params(GuiApp* self, GtkGrid* grid, int& row);
    static void add_control_panel(GuiApp* self, GtkBox* left_box);
    static void add_state_and_disturbance(GuiApp* self, GtkBox* left_box);
    static void add_tracking_controls(GuiApp* self, GtkBox* left_box);
    static void add_param_row(GuiApp* self, GtkGrid* grid,
        const char* name, GtkWidget* entry, const char* initial_val,
        int r, GtkWidget* step_entry, const char* tip_label,
        const char* tip_inc, const char* tip_dec);

    static double get_double_from_entry(GtkWidget* entry, bool* ok = nullptr);
    static void set_double_to_entry(GtkWidget* entry, double val);
    static void update_state_label(GuiApp* self, int state);
    static void update_ui_interval(GuiApp* self, int ms);
    static void update_status_bar(GuiApp* self);
    static void apply_toggle_css(GtkWidget* btn, bool active);
};