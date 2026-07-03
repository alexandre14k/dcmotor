// src/gui/gui.cpp
#include "gui.hpp"
#include "label.hpp"
#include "tooltip.hpp"

GuiApp::GuiApp(Motor& m, Logger& l) : app(nullptr),
    window(nullptr), provider(nullptr), motor(m), logger(l) {}

GuiApp::~GuiApp() {
    if(app) g_object_unref(app);
}

int GuiApp::run(int argc, char** argv) {
    app = gtk_application_new("org.motor",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate",
        G_CALLBACK(on_activate), this);
    int status = g_application_run(
        G_APPLICATION(app), argc, argv);
    return status;
}

double GuiApp::get_double_from_entry(GtkWidget* entry, bool* ok) {
    if (!entry) {
        if (ok) *ok = false;
        return 0.0;
    }
    const gchar* text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!text || text[0] == '\0') {
        if (ok) *ok = false;
        return 0.0;
    }
    gchar* endptr = nullptr;
    double val = g_ascii_strtod(text, &endptr);
    if (endptr == text) {
        if (ok) *ok = false;
        return 0.0;
    }
    if (ok) *ok = true;
    return val;
}

void GuiApp::set_double_to_entry(GtkWidget* entry, double val) {
    if (!entry) return;
    m_string s = fmt::format("{:.6f}", val);
    if (s.find('.') != m_string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    gtk_editable_set_text(GTK_EDITABLE(entry), s.c_str());
}

void GuiApp::setup_css_provider(GuiApp* self) {
    if (!self) return;
    self->provider = gtk_css_provider_new();
    m_string css =
        ".state-good { background-color: #4caf50; "
        "color: white; font-weight: bold; }"
        ".state-warning { background-color: #ffeb3b; "
        "color: black; font-weight: bold; }"
        ".state-saturation { background-color: #f44336; "
        "color: white; font-weight: bold; }"
        ".toggle-active { background-color: orange; "
        "color: black; font-weight: bold; }"
        ".toggle-active:hover { background-color: #cc8400; }";
    gtk_css_provider_load_from_string(self->provider, css.c_str());
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(self->provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

GtkWidget* GuiApp::create_main_layout(GuiApp* self) {
    if (!self) return nullptr;
    self->main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(self->window), self->main_box);
    return self->main_box;
}

GtkWidget* GuiApp::create_left_panel(GuiApp* self) {
    if (!self) return nullptr;
    GtkWidget* left_box = gtk_box_new(
        GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(left_box, 10);
    gtk_widget_set_margin_bottom(left_box, 10);
    gtk_widget_set_margin_start(left_box, 10);
    gtk_widget_set_margin_end(left_box, 10);

    self->pwm_label = gtk_label_new(LBL_PWM_REF);
    gtk_box_append(GTK_BOX(left_box), self->pwm_label);

    self->slider = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_tooltip_text(self->slider, TIP_PWM_REF);
    gtk_widget_set_size_request(self->slider, 300, -1);
    g_signal_connect(self->slider, "value-changed",
        G_CALLBACK(on_slider_changed), self);
    gtk_box_append(GTK_BOX(left_box), self->slider);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_box_append(GTK_BOX(left_box), grid);

    int row = 0;
    add_pid_params(self, GTK_GRID(grid), row);
    add_motor_params(self, GTK_GRID(grid), row);
    add_control_panel(self, GTK_BOX(left_box));
    add_state_and_disturbance(self, GTK_BOX(left_box));
    add_tracking_controls(self, GTK_BOX(left_box));

    self->rpm_label = gtk_label_new("0 RPM");
    gtk_box_append(GTK_BOX(left_box), self->rpm_label);

    self->rpm_bar = gtk_progress_bar_new();
    gtk_widget_set_tooltip_text(self->rpm_bar, TIP_RPM_BAR);
    gtk_box_append(GTK_BOX(left_box), self->rpm_bar);

    self->pause_btn = gtk_toggle_button_new_with_label(LBL_PAUSE);
    gtk_widget_set_tooltip_text(self->pause_btn, TIP_PAUSE);
    g_signal_connect(self->pause_btn, "toggled",
        G_CALLBACK(on_pause_toggled), self);
    gtk_box_append(GTK_BOX(left_box), self->pause_btn);

    GtkWidget* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);

    GtkWidget* url_btn = gtk_button_new_with_label(LBL_LEARN_MORE);
    gtk_widget_set_tooltip_text(url_btn, TIP_URL);
    g_signal_connect(url_btn, "clicked",
        G_CALLBACK(on_url_clicked), nullptr);
    gtk_box_append(GTK_BOX(btn_box), url_btn);

    GtkWidget* ss_btn = gtk_button_new_with_label("Save Screenshot");
    gtk_widget_set_tooltip_text(ss_btn, "Save a PNG screenshot of the window");
    g_signal_connect(ss_btn, "clicked",
        G_CALLBACK(on_screenshot_clicked), self);
    gtk_box_append(GTK_BOX(btn_box), ss_btn);

    gtk_box_append(GTK_BOX(left_box), btn_box);

    self->debug_btn = gtk_toggle_button_new_with_label(LBL_DEBUG);
    gtk_widget_set_tooltip_text(self->debug_btn, TIP_DEBUG);
    g_signal_connect(self->debug_btn, "toggled",
        G_CALLBACK(on_debug_toggled), self);
    gtk_box_append(GTK_BOX(left_box), self->debug_btn);

    self->speed_slider = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 2, 1);
    gtk_scale_set_draw_value(
        GTK_SCALE(self->speed_slider), FALSE);
    gtk_widget_set_tooltip_text(self->speed_slider, TIP_SPEED);
    gtk_widget_set_size_request(self->speed_slider, 300, -1);
    gtk_range_set_value(GTK_RANGE(self->speed_slider), 1);
    g_signal_connect(self->speed_slider, "value-changed",
        G_CALLBACK(on_speed_changed), self);
    g_signal_connect(self->speed_slider, "change-value",
        G_CALLBACK(on_speed_change_value), self);
    gtk_box_append(GTK_BOX(left_box), self->speed_slider);

    GtkWidget* time_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(time_btn_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(time_btn_box, 5);

    self->time_compress_btn = gtk_button_new_with_label(LBL_TIME_COMPRESS);
    gtk_widget_set_tooltip_text(self->time_compress_btn, TIP_TIME_COMPRESS);
    g_signal_connect(self->time_compress_btn, "clicked",
        G_CALLBACK(on_time_compress_clicked), self);
    gtk_box_append(GTK_BOX(time_btn_box), self->time_compress_btn);

    self->time_expand_btn = gtk_button_new_with_label(LBL_TIME_EXPAND);
    gtk_widget_set_tooltip_text(self->time_expand_btn, TIP_TIME_EXPAND);
    g_signal_connect(self->time_expand_btn, "clicked",
        G_CALLBACK(on_time_expand_clicked), self);
    gtk_box_append(GTK_BOX(time_btn_box), self->time_expand_btn);

    gtk_box_append(GTK_BOX(left_box), time_btn_box);

    self->status_label = gtk_label_new("");
    gtk_widget_set_halign(self->status_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(left_box), self->status_label);

    return left_box;
}

void GuiApp::add_pid_params(GuiApp* self, GtkGrid* grid, int& row) {
    if (!self || !grid) return;
    gtk_grid_attach(grid, gtk_label_new(LBL_PID), 0, row, 2, 1);
    gtk_grid_attach(grid, gtk_label_new(LBL_STEP), 2, row, 1, 1);
    self->pid_step_entry = gtk_entry_new();
    gtk_widget_set_tooltip_text(self->pid_step_entry, TIP_STEP);
    gtk_editable_set_text(GTK_EDITABLE(self->pid_step_entry), "0.1");
    gtk_widget_set_size_request(self->pid_step_entry, 80, -1);
    gtk_grid_attach(grid, self->pid_step_entry, 3, row, 1, 1);
    row++;

    self->kp_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->kp_entry), "0.1");
    gtk_widget_set_size_request(self->kp_entry, 80, -1);
    g_signal_connect(self->kp_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_KP, self->kp_entry, "0.1",
        row++, self->pid_step_entry, TIP_KP, TIP_KP_INC, TIP_KP_DEC);

    self->ki_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->ki_entry), "1.0");
    gtk_widget_set_size_request(self->ki_entry, 80, -1);
    g_signal_connect(self->ki_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_KI, self->ki_entry, "1.0",
        row++, self->pid_step_entry, TIP_KI, TIP_KI_INC, TIP_KI_DEC);

    self->kd_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->kd_entry), "0.001");
    gtk_widget_set_size_request(self->kd_entry, 80, -1);
    g_signal_connect(self->kd_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_KD, self->kd_entry, "0.001",
        row++, self->pid_step_entry, TIP_KD, TIP_KD_INC, TIP_KD_DEC);
}

void GuiApp::add_motor_params(GuiApp* self, GtkGrid* grid, int& row) {
    if (!self || !grid) return;

    gtk_grid_attach(grid, gtk_label_new("Motor Type:"), 0, row, 1, 1);
    const char* motor_types[] = {
        "Permanent-magnet brushed DC (PMDC)",
        "Coreless brushed DC",
        "Wound-field DC (fixed field current)",
        "Shunt DC (fixed field excitation)",
        "Compound DC (fixed field excitation)",
        NULL
    };
    self->motor_type_combo = gtk_drop_down_new_from_strings(motor_types);
    gtk_widget_set_hexpand(self->motor_type_combo, TRUE);
    gtk_widget_set_tooltip_text(self->motor_type_combo, "Select motor type to apply preset physical parameters");
    g_signal_connect(self->motor_type_combo, "notify::selected",
        G_CALLBACK(on_motor_type_changed), self);
    gtk_grid_attach(grid, self->motor_type_combo, 1, row, 3, 1);
    row++;

    gtk_grid_attach(grid, gtk_label_new(LBL_MOTOR), 0, row, 2, 1);
    gtk_grid_attach(grid, gtk_label_new(LBL_STEP), 2, row, 1, 1);
    self->motor_step_entry = gtk_entry_new();
    gtk_widget_set_tooltip_text(self->motor_step_entry, TIP_STEP);
    gtk_editable_set_text(GTK_EDITABLE(self->motor_step_entry), "0.1");
    gtk_widget_set_size_request(self->motor_step_entry, 80, -1);
    gtk_grid_attach(grid, self->motor_step_entry, 3, row, 1, 1);
    row++;

    self->r_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->r_entry), "1.0");
    gtk_widget_set_size_request(self->r_entry, 80, -1);
    g_signal_connect(self->r_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_R, self->r_entry, "1.0",
        row++, self->motor_step_entry, TIP_R, TIP_R_INC, TIP_R_DEC);

    self->j_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->j_entry), "0.001");
    gtk_widget_set_size_request(self->j_entry, 80, -1);
    g_signal_connect(self->j_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_J, self->j_entry, "0.001",
        row++, self->motor_step_entry, TIP_J, TIP_J_INC, TIP_J_DEC);

    self->b_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->b_entry), "0.00001");
    gtk_widget_set_size_request(self->b_entry, 80, -1);
    g_signal_connect(self->b_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_B, self->b_entry, "0.00001",
        row++, self->motor_step_entry, TIP_B, TIP_B_INC, TIP_B_DEC);

    self->kt_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->kt_entry), "0.02");
    gtk_widget_set_size_request(self->kt_entry, 80, -1);
    g_signal_connect(self->kt_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_KT, self->kt_entry, "0.02",
        row++, self->motor_step_entry, TIP_KT, TIP_KT_INC, TIP_KT_DEC);

    self->ke_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->ke_entry), "0.02");
    gtk_widget_set_size_request(self->ke_entry, 80, -1);
    g_signal_connect(self->ke_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    add_param_row(self, grid, LBL_KE, self->ke_entry, "0.02",
        row++, self->motor_step_entry, TIP_KE, TIP_KE_INC, TIP_KE_DEC);
}

void GuiApp::on_motor_type_changed(GObject* obj, GParamSpec* pspec, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;

    guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));

    g_signal_handlers_block_by_func(self->r_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_block_by_func(self->j_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_block_by_func(self->b_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_block_by_func(self->kt_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_block_by_func(self->ke_entry, (gpointer)on_param_changed, self);

    switch(selected) {
        case 0: // PMDC
            set_double_to_entry(self->r_entry, 1.0);
            set_double_to_entry(self->j_entry, 0.001);
            set_double_to_entry(self->b_entry, 0.00001);
            set_double_to_entry(self->kt_entry, 0.02);
            set_double_to_entry(self->ke_entry, 0.02);
            break;
        case 1: // Coreless
            set_double_to_entry(self->r_entry, 0.5);
            set_double_to_entry(self->j_entry, 0.0001);
            set_double_to_entry(self->b_entry, 0.000001);
            set_double_to_entry(self->kt_entry, 0.01);
            set_double_to_entry(self->ke_entry, 0.01);
            break;
        case 2: // Wound-field
            set_double_to_entry(self->r_entry, 2.0);
            set_double_to_entry(self->j_entry, 0.005);
            set_double_to_entry(self->b_entry, 0.00005);
            set_double_to_entry(self->kt_entry, 0.05);
            set_double_to_entry(self->ke_entry, 0.05);
            break;
        case 3: // Shunt
            set_double_to_entry(self->r_entry, 5.0);
            set_double_to_entry(self->j_entry, 0.01);
            set_double_to_entry(self->b_entry, 0.0001);
            set_double_to_entry(self->kt_entry, 0.08);
            set_double_to_entry(self->ke_entry, 0.08);
            break;
        case 4: // Compound
            set_double_to_entry(self->r_entry, 8.0);
            set_double_to_entry(self->j_entry, 0.05);
            set_double_to_entry(self->b_entry, 0.005);
            set_double_to_entry(self->kt_entry, 0.09);
            set_double_to_entry(self->ke_entry, 0.09);
            break;
    }

    g_signal_handlers_unblock_by_func(self->r_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_unblock_by_func(self->j_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_unblock_by_func(self->b_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_unblock_by_func(self->kt_entry, (gpointer)on_param_changed, self);
    g_signal_handlers_unblock_by_func(self->ke_entry, (gpointer)on_param_changed, self);

    on_param_changed(GTK_EDITABLE(self->r_entry), self);
    self->logger.log(fmt::format("{},GUI,motor_type,{}", self->logger.get_ms(), selected));
}

void GuiApp::add_param_row(GuiApp* self, GtkGrid* grid, const char* name,
    GtkWidget* entry, const char* initial_val, int r, GtkWidget* step_entry,
    const char* tip_label, const char* tip_inc, const char* tip_dec) {

    if (!self || !grid || !entry || !step_entry) return;

    GtkWidget* lbl = gtk_label_new(name);
    gtk_widget_set_tooltip_text(lbl, tip_label);
    gtk_grid_attach(grid, lbl, 0, r, 1, 1);
    gtk_grid_attach(grid, entry, 1, r, 1, 1);

    GtkWidget* plus = gtk_button_new_with_label(LBL_PLUS);
    g_object_set_data(G_OBJECT(plus), "entry", entry);
    g_object_set_data(G_OBJECT(plus), "step_entry", step_entry);
    g_signal_connect(plus, "clicked", G_CALLBACK(on_param_inc), self);
    gtk_widget_set_tooltip_text(plus, tip_inc);
    gtk_grid_attach(grid, plus, 2, r, 1, 1);

    GtkWidget* minus = gtk_button_new_with_label(LBL_MINUS);
    g_object_set_data(G_OBJECT(minus), "entry", entry);
    g_object_set_data(G_OBJECT(minus), "step_entry", step_entry);
    g_signal_connect(minus, "clicked", G_CALLBACK(on_param_dec), self);
    gtk_widget_set_tooltip_text(minus, tip_dec);
    gtk_grid_attach(grid, minus, 3, r, 1, 1);

    GtkWidget* reset = gtk_button_new_with_label(LBL_RESET);
    g_object_set_data(G_OBJECT(reset), "entry", entry);
    g_object_set_data(G_OBJECT(reset), "initial_val", (gpointer)initial_val);
    g_signal_connect(reset, "clicked", G_CALLBACK(on_param_reset), self);
    gtk_widget_set_tooltip_text(reset, TIP_RESET);
    gtk_grid_attach(grid, reset, 4, r, 1, 1);
}

void GuiApp::add_control_panel(GuiApp* self, GtkBox* left_box) {
    if (!self || !left_box) return;
    GtkWidget* ctrl_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(ctrl_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(ctrl_box, 10);

    self->noise_btn = gtk_toggle_button_new_with_label(LBL_NOISE);
    gtk_widget_set_tooltip_text(self->noise_btn, TIP_NOISE);
    g_signal_connect(self->noise_btn, "toggled",
        G_CALLBACK(on_noise_toggled), self);
    gtk_box_append(GTK_BOX(ctrl_box), self->noise_btn);

    self->noise_amp_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->noise_amp_entry), "0.001");
    gtk_widget_set_tooltip_text(self->noise_amp_entry, TIP_NOISE_AMP);
    gtk_widget_set_size_request(self->noise_amp_entry, 80, -1);
    g_signal_connect(self->noise_amp_entry, "changed",
        G_CALLBACK(on_param_changed), self);
    gtk_box_append(GTK_BOX(ctrl_box), self->noise_amp_entry);

    self->log_btn = gtk_toggle_button_new_with_label(LBL_LOG);
    gtk_widget_set_tooltip_text(self->log_btn, TIP_LOG);
    g_signal_connect(self->log_btn, "toggled",
        G_CALLBACK(on_log_toggled), self);
    gtk_box_append(GTK_BOX(ctrl_box), self->log_btn);

    gtk_box_append(GTK_BOX(left_box), ctrl_box);
}

void GuiApp::add_state_and_disturbance(GuiApp* self, GtkBox* left_box) {
    if (!self || !left_box) return;

    GtkWidget* disturb_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(disturb_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(disturb_box, 10);
    gtk_widget_set_margin_bottom(disturb_box, 10);

    GtkWidget* disturb_pwm_btn = gtk_button_new_with_label(LBL_DISTURB_PWM);
    gtk_widget_set_tooltip_text(disturb_pwm_btn, TIP_DISTURB_PWM);
    g_signal_connect(disturb_pwm_btn, "clicked",
        G_CALLBACK(on_disturb_pwm_clicked), self);
    gtk_box_append(GTK_BOX(disturb_box), disturb_pwm_btn);

    self->state_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(self->state_box, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(self->state_box, TIP_STATE);
    gtk_widget_add_css_class(self->state_box, "state-good");

    self->state_label = gtk_label_new(LBL_GOOD);
    gtk_widget_set_margin_top(self->state_label, 10);
    gtk_widget_set_margin_bottom(self->state_label, 10);
    gtk_widget_set_margin_start(self->state_label, 40);
    gtk_widget_set_margin_end(self->state_label, 40);
    gtk_box_append(GTK_BOX(self->state_box), self->state_label);

    gtk_box_append(GTK_BOX(disturb_box), self->state_box);

    GtkWidget* disturb_rpm_btn = gtk_button_new_with_label(LBL_DISTURB_RPM);
    gtk_widget_set_tooltip_text(disturb_rpm_btn, TIP_DISTURB_RPM);
    g_signal_connect(disturb_rpm_btn, "clicked",
        G_CALLBACK(on_disturb_rpm_clicked), self);
    gtk_box_append(GTK_BOX(disturb_box), disturb_rpm_btn);

    gtk_box_append(GTK_BOX(left_box), disturb_box);
}

void GuiApp::add_tracking_controls(GuiApp* self, GtkBox* left_box) {
    if (!self || !left_box) return;
    GtkWidget* track_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(track_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(track_box, 10);

    self->track_step_down_btn = gtk_button_new_with_label("Step Down");
    gtk_widget_set_tooltip_text(self->track_step_down_btn, TIP_TRACK_STEP_DOWN);
    g_signal_connect(self->track_step_down_btn, "clicked",
        G_CALLBACK(on_track_step_down_clicked), self);
    gtk_box_append(GTK_BOX(track_box), self->track_step_down_btn);

    gtk_box_append(GTK_BOX(track_box), gtk_label_new(LBL_TRACK));

    self->track_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(self->track_entry), "1000");
    gtk_widget_set_tooltip_text(self->track_entry, TIP_TRACK_ENTRY);
    gtk_widget_set_size_request(self->track_entry, 80, -1);
    g_signal_connect(self->track_entry, "changed",
        G_CALLBACK(on_track_changed), self);
    gtk_box_append(GTK_BOX(track_box), self->track_entry);

    self->track_btn = gtk_toggle_button_new_with_label(LBL_TRACK_BTN);
    gtk_widget_set_tooltip_text(self->track_btn, TIP_TRACK);
    g_signal_connect(self->track_btn, "toggled",
        G_CALLBACK(on_track_toggled), self);
    gtk_box_append(GTK_BOX(track_box), self->track_btn);

    self->track_step_up_btn = gtk_button_new_with_label("Step Up");
    gtk_widget_set_tooltip_text(self->track_step_up_btn, TIP_TRACK_STEP_UP);
    g_signal_connect(self->track_step_up_btn, "clicked",
        G_CALLBACK(on_track_step_up_clicked), self);
    gtk_box_append(GTK_BOX(track_box), self->track_step_up_btn);

    gtk_box_append(GTK_BOX(left_box), track_box);
}

void GuiApp::on_activate(GtkApplication* app, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;

    self->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(self->window), APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(self->window), 800, 500);

    setup_css_provider(self);

    GtkWidget* main_box = create_main_layout(self);
    self->left_box = create_left_panel(self);
    gtk_box_append(GTK_BOX(main_box), self->left_box);

    // Right side vertical box to hold the button row and the plot
    self->right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(main_box), self->right_box);

    // Horizontal box to hold the buttons in their own row
    self->plot_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(self->plot_btn_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(self->plot_btn_box, 10);
    gtk_widget_set_margin_end(self->plot_btn_box, 10);
    gtk_box_append(GTK_BOX(self->right_box), self->plot_btn_box);

    self->fullscreen_btn = gtk_toggle_button_new_with_label("Fullscreen");
    gtk_widget_set_tooltip_text(self->fullscreen_btn, "Toggle fullscreen plot view");
    g_signal_connect(self->fullscreen_btn, "toggled",
        G_CALLBACK(on_fullscreen_toggled), self);
    gtk_box_append(GTK_BOX(self->plot_btn_box), self->fullscreen_btn);

    self->track_btn_overlay = gtk_toggle_button_new_with_label("Track");
    gtk_widget_set_tooltip_text(self->track_btn_overlay, "Toggle RPM tracking");
    g_signal_connect(self->track_btn_overlay, "toggled",
        G_CALLBACK(on_track_toggled_overlay), self);
    gtk_box_append(GTK_BOX(self->plot_btn_box), self->track_btn_overlay);

    self->screenshot_btn_overlay = gtk_toggle_button_new_with_label("Screenshot");
    gtk_widget_set_tooltip_text(self->screenshot_btn_overlay, "Save a PNG screenshot");
    g_signal_connect(self->screenshot_btn_overlay, "toggled",
        G_CALLBACK(on_screenshot_toggled_overlay), self);
    gtk_box_append(GTK_BOX(self->plot_btn_box), self->screenshot_btn_overlay);

    // Add plot below the buttons, making it expand vertically
    GtkWidget* plot_widget = self->plot.get_widget();
    gtk_widget_set_vexpand(plot_widget, TRUE);
    gtk_widget_set_valign(plot_widget, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(self->right_box), plot_widget);

    update_ui_interval(self, NORMAL_MODE_MS);
    update_status_bar(self);
    gtk_window_present(GTK_WINDOW(self->window));
}

void GuiApp::on_param_inc(GtkButton* btn, gpointer data) {
    if (!btn || !data) return;
    GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "entry"));
    GtkWidget* step_entry = GTK_WIDGET(
        g_object_get_data(G_OBJECT(btn), "step_entry"));
    if (!entry || !step_entry) return;

    double val = get_double_from_entry(entry);
    double step = get_double_from_entry(step_entry);
    val += step;
    set_double_to_entry(entry, val);
}

void GuiApp::on_param_dec(GtkButton* btn, gpointer data) {
    if (!btn || !data) return;
    GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "entry"));
    GtkWidget* step_entry = GTK_WIDGET(
        g_object_get_data(G_OBJECT(btn), "step_entry"));
    if (!entry || !step_entry) return;

    double val = get_double_from_entry(entry);
    double step = get_double_from_entry(step_entry);
    val -= step;
    if(val < 0) val = 0;
    set_double_to_entry(entry, val);
}

void GuiApp::on_param_reset(GtkButton* btn, gpointer data) {
    if (!btn || !data) return;
    GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "entry"));
    const char* initial_val = (const char*)g_object_get_data(
        G_OBJECT(btn), "initial_val");
    if (!entry || !initial_val) return;

    gtk_editable_set_text(GTK_EDITABLE(entry), initial_val);
}

void GuiApp::apply_toggle_css(GtkWidget* btn, bool active) {
    if (!btn) return;
    if (active) {
        gtk_widget_add_css_class(btn, "toggle-active");
    } else {
        gtk_widget_remove_css_class(btn, "toggle-active");
    }
}

void GuiApp::on_noise_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_NOISE_PARAMS;
    msg.noise_enabled = active;
    msg.noise_amp = get_double_from_entry(self->noise_amp_entry);
    self->motor.get_motor_q().push(msg);
    update_status_bar(self);
}

void GuiApp::on_log_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);
    self->plot.set_log_view(active);
    update_status_bar(self);
}

void GuiApp::on_track_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);

    // Sync overlay button
    g_signal_handlers_block_by_func(self->track_btn_overlay, (gpointer)on_track_toggled_overlay, self);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->track_btn_overlay), active);
    apply_toggle_css(self->track_btn_overlay, active);
    g_signal_handlers_unblock_by_func(self->track_btn_overlay, (gpointer)on_track_toggled_overlay, self);

    double track_rpm = get_double_from_entry(self->track_entry);
    GuiToMotor msg;
    msg.type = GuiToMotor::SET_TRACK_PARAMS;
    msg.track_enabled = active;
    msg.track_rpm = track_rpm;
    self->motor.get_motor_q().push(msg);

    self->plot.set_tracking(active, track_rpm);
    update_status_bar(self);
}

void GuiApp::on_track_toggled_overlay(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);

    // Sync main panel button
    g_signal_handlers_block_by_func(self->track_btn, (gpointer)on_track_toggled, self);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->track_btn), active);
    apply_toggle_css(self->track_btn, active);
    g_signal_handlers_unblock_by_func(self->track_btn, (gpointer)on_track_toggled, self);

    double track_rpm = get_double_from_entry(self->track_entry);
    GuiToMotor msg;
    msg.type = GuiToMotor::SET_TRACK_PARAMS;
    msg.track_enabled = active;
    msg.track_rpm = track_rpm;
    self->motor.get_motor_q().push(msg);

    self->plot.set_tracking(active, track_rpm);
    update_status_bar(self);
}

void GuiApp::on_track_changed(GtkEditable* edit, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    double track_rpm = get_double_from_entry(self->track_entry);
    bool active = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(self->track_btn));

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_TRACK_PARAMS;
    msg.track_enabled = active;
    msg.track_rpm = track_rpm;
    self->motor.get_motor_q().push(msg);

    self->plot.set_tracking(active, track_rpm);
}

void GuiApp::on_track_step_up_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    double val = get_double_from_entry(self->track_entry);
    val += 100.0;
    if (val > 5000.0) val = 5000.0;
    set_double_to_entry(self->track_entry, val);
}

void GuiApp::on_track_step_down_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    double val = get_double_from_entry(self->track_entry);
    val -= 100.0;
    if (val < 0.0) val = 0.0;
    set_double_to_entry(self->track_entry, val);
}

void GuiApp::on_pause_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_PAUSE;
    msg.paused = active;
    self->motor.get_motor_q().push(msg);
    update_status_bar(self);
}

void GuiApp::on_debug_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);
    self->logger.set_debug_enabled(active);
}

void GuiApp::on_fullscreen_toggled(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    bool active = gtk_toggle_button_get_active(btn);
    apply_toggle_css(GTK_WIDGET(btn), active);
    gtk_widget_set_visible(self->left_box, !active);
}

void GuiApp::on_screenshot_toggled_overlay(GtkToggleButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    if (gtk_toggle_button_get_active(btn)) {
        on_screenshot_clicked(GTK_BUTTON(btn), self);
        // Immediately turn off to behave like a momentary push button
        g_signal_handlers_block_by_func(btn, (gpointer)on_screenshot_toggled_overlay, self);
        gtk_toggle_button_set_active(btn, FALSE);
        g_signal_handlers_unblock_by_func(btn, (gpointer)on_screenshot_toggled_overlay, self);
    }
}

void GuiApp::on_speed_changed(GtkRange* range, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    int val = gtk_range_get_value(range);
    if (val == 0) update_ui_interval(self, SLOW_MODE_MS);
    else if (val == 1) update_ui_interval(self, NORMAL_MODE_MS);
    else if (val == 2) update_ui_interval(self, FAST_MODE_MS);
    update_status_bar(self);
}

gboolean GuiApp::on_speed_change_value(GtkRange* range, GtkScrollType scroll, gdouble value, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return FALSE;

    int new_val = (int)std::round(value);
    if (new_val < 0) new_val = 0;
    if (new_val > 2) new_val = 2;

    if (gtk_range_get_value(range) != new_val) {
        gtk_range_set_value(range, new_val);
    }
    return TRUE;
}

void GuiApp::on_time_expand_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    self->plot.adjust_time_window(5.0);
}

void GuiApp::on_time_compress_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    self->plot.adjust_time_window(-5.0);
}

void GuiApp::update_ui_interval(GuiApp* self, int ms) {
    if (!self) return;
    if (self->ui_timeout_id > 0) g_source_remove(self->ui_timeout_id);
    self->ui_timeout_id = g_timeout_add(ms, on_update_ui, self);
}

void GuiApp::update_status_bar(GuiApp* self) {
    if (!self || !self->status_label) return;
    m_string status = "";

    int speed_val = gtk_range_get_value(GTK_RANGE(self->speed_slider));
    if (speed_val == 0) status += "Speed: Slow";
    else if (speed_val == 1) status += "Speed: Normal";
    else if (speed_val == 2) status += "Speed: Fast";

    bool paused = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(self->pause_btn));
    status += paused ? " | Paused" : " | Running";

    bool track = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(self->track_btn));
    status += track ? " | Tracking" : " | No Track";

    bool noise = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(self->noise_btn));
    status += noise ? " | Noise On" : " | Noise Off";

    bool log = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(self->log_btn));
    status += log ? " | Log Plot" : " | Lin Plot";

    gtk_label_set_text(GTK_LABEL(self->status_label), status.c_str());
}

void GuiApp::on_screenshot_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self || !self->window) return;

    GtkWidget* child = gtk_window_get_child(GTK_WINDOW(self->window));
    if (!child) return;

    int width = gtk_widget_get_width(self->window);
    int height = gtk_widget_get_height(self->window);
    if (width <= 0 || height <= 0) return;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    // Detect theme using gtk_widget_get_color and paint background to fix transparency
    GdkRGBA fg_color;
    gtk_widget_get_color(self->window, &fg_color);
    bool is_dark = (fg_color.red + fg_color.green + fg_color.blue) / 3.0 > 0.5;

    if (is_dark) {
        cairo_set_source_rgb(cr, 0.12, 0.12, 0.12); // Dark theme background
    } else {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);    // Light theme background
    }
    cairo_paint(cr);

    GtkSnapshot* snapshot = gtk_snapshot_new();
    gtk_widget_snapshot_child(self->window, child, snapshot);
    GskRenderNode* node = gtk_snapshot_free_to_node(snapshot);

    if (node) {
        graphene_rect_t bounds;
        gsk_render_node_get_bounds(node, &bounds);
        cairo_translate(cr, bounds.origin.x, bounds.origin.y);
        gsk_render_node_draw(node, cr);
        gsk_render_node_unref(node);
    }

    cairo_destroy(cr);

    GDateTime* now = g_date_time_new_now_local();
    char* time_str = g_date_time_format(now, "%Y-%m-%d_%H-%M-%S");
    m_string filename = fmt::format("screenshot_{}.png", time_str);
    g_free(time_str);
    g_date_time_unref(now);

    cairo_status_t status = cairo_surface_write_to_png(surface, filename.c_str());
    cairo_surface_destroy(surface);

    if (status == CAIRO_STATUS_SUCCESS) {
        self->logger.log(fmt::format("{},GUI,screenshot,{}", self->logger.get_ms(), filename));
    } else {
        self->logger.log(fmt::format("{},GUI,screenshot_error,{}", self->logger.get_ms(), status));
    }
}

void GuiApp::on_disturb_pwm_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;

    if (self->pwm_disturb_timeout_id > 0) {
        g_source_remove(self->pwm_disturb_timeout_id);
        self->pwm_disturb_timeout_id = 0;
    }

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_PWM_DISTURBANCE;
    msg.pwm_disturbance = 0.6;
    self->motor.get_motor_q().push(msg);

    self->pwm_disturb_timeout_id = g_timeout_add(200,
        reset_pwm_disturbance, self);
}

gboolean GuiApp::reset_pwm_disturbance(gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return G_SOURCE_REMOVE;

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_PWM_DISTURBANCE;
    msg.pwm_disturbance = 0.0;
    self->motor.get_motor_q().push(msg);

    self->pwm_disturb_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void GuiApp::on_disturb_rpm_clicked(GtkButton* btn, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;

    if (self->rpm_disturb_timeout_id > 0) {
        g_source_remove(self->rpm_disturb_timeout_id);
        self->rpm_disturb_timeout_id = 0;
    }

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_RPM_DISTURBANCE;
    msg.rpm_disturbance = 0.05;
    self->motor.get_motor_q().push(msg);

    self->rpm_disturb_timeout_id = g_timeout_add(200,
        reset_rpm_disturbance, self);
}

gboolean GuiApp::reset_rpm_disturbance(gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return G_SOURCE_REMOVE;

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_RPM_DISTURBANCE;
    msg.rpm_disturbance = 0.0;
    self->motor.get_motor_q().push(msg);

    self->rpm_disturb_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void GuiApp::on_url_clicked(GtkButton* btn, gpointer data) {
    g_app_info_launch_default_for_uri(APP_URL, nullptr, nullptr);
}

void GuiApp::on_param_changed(GtkEditable* edit, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;

    bool ok = true;
    double kp = get_double_from_entry(self->kp_entry, &ok);
    double ki = get_double_from_entry(self->ki_entry, &ok);
    double kd = get_double_from_entry(self->kd_entry, &ok);

    if (ok) {
        auto clamp = [](double v, double min, double max) {
            return std::max(min, std::min(max, v));
        };
        GuiToMotor pid_msg;
        pid_msg.type = GuiToMotor::SET_PID_PARAMS;
        pid_msg.pid_params.Kp = clamp(kp, 0.0, 10.0);
        pid_msg.pid_params.Ki = clamp(ki, 0.0, 10.0);
        pid_msg.pid_params.Kd = clamp(kd, 0.0, 10.0);
        self->motor.get_motor_q().push(pid_msg);
    }

    ok = true;
    double r = get_double_from_entry(self->r_entry, &ok);
    double j = get_double_from_entry(self->j_entry, &ok);
    double b = get_double_from_entry(self->b_entry, &ok);
    double kt = get_double_from_entry(self->kt_entry, &ok);
    double ke = get_double_from_entry(self->ke_entry, &ok);

    if (ok) {
        auto clamp = [](double v, double min, double max) {
            return std::max(min, std::min(max, v));
        };
        GuiToMotor plant_msg;
        plant_msg.type = GuiToMotor::SET_PLANT_PARAMS;
        plant_msg.plant_params.R = clamp(r, 0.1, 10.0);
        plant_msg.plant_params.J = clamp(j, 0.0001, 0.1);
        plant_msg.plant_params.B = clamp(b, 0.000001, 0.01);
        plant_msg.plant_params.Kt = clamp(kt, 0.001, 0.1);
        plant_msg.plant_params.Ke = clamp(ke, 0.001, 0.1);
        self->motor.get_motor_q().push(plant_msg);
    }

    ok = true;
    double noise_amp = get_double_from_entry(self->noise_amp_entry, &ok);
    if (ok) {
        auto clamp = [](double v, double min, double max) {
            return std::max(min, std::min(max, v));
        };
        GuiToMotor noise_msg;
        noise_msg.type = GuiToMotor::SET_NOISE_PARAMS;
        noise_msg.noise_enabled = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(self->noise_btn));
        noise_msg.noise_amp = clamp(noise_amp, 0.001, 1.0);
        self->motor.get_motor_q().push(noise_msg);
    }

    self->logger.log(fmt::format("{},GUI,params",
        self->logger.get_ms()));
}

void GuiApp::on_slider_changed(GtkRange* range, gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return;
    int val = gtk_range_get_value(range);
    m_string text = fmt::format("PWM Reference Point: {}%", val);
    gtk_label_set_label(GTK_LABEL(self->pwm_label), text.c_str());

    if(val != self->last_slider_val) {
        self->logger.log(fmt::format("{},GUI,slider,{}",
            self->logger.get_ms(), val));
        self->last_slider_val = val;
    }

    GuiToMotor msg;
    msg.type = GuiToMotor::SET_TARGET;
    msg.target_rpm = val;
    self->motor.get_motor_q().push(msg);
}

void GuiApp::update_state_label(GuiApp* self, int state) {
    if (!self || !GTK_IS_WIDGET(self->state_box) ||
        !GTK_IS_WIDGET(self->state_label)) return;

    if (state == 2) {
        gtk_widget_remove_css_class(self->state_box, "state-good");
        gtk_widget_remove_css_class(self->state_box, "state-warning");
        gtk_widget_add_css_class(self->state_box, "state-saturation");
        gtk_label_set_label(GTK_LABEL(self->state_label), "saturation");
    } else if (state == 1) {
        gtk_widget_remove_css_class(self->state_box, "state-good");
        gtk_widget_remove_css_class(self->state_box, "state-saturation");
        gtk_widget_add_css_class(self->state_box, "state-warning");
        gtk_label_set_label(GTK_LABEL(self->state_label), "warning");
    } else {
        gtk_widget_remove_css_class(self->state_box, "state-warning");
        gtk_widget_remove_css_class(self->state_box, "state-saturation");
        gtk_widget_add_css_class(self->state_box, "state-good");
        gtk_label_set_label(GTK_LABEL(self->state_label), LBL_GOOD);
    }
}

gboolean GuiApp::on_update_ui(gpointer data) {
    GuiApp* self = static_cast<GuiApp*>(data);
    if (!self) return G_SOURCE_CONTINUE;

    MotorToGui msg;
    int state = 0;
    double pwm = 0, rpm = 0, Kp = 0, Ki = 0, Kd = 0;
    while(self->motor.get_gui_q().pop(msg)) {
        rpm = msg.rpm;
        state = msg.state;
        pwm = msg.pwm;
        Kp = msg.Kp;
        Ki = msg.Ki;
        Kd = msg.Kd;
    }

    auto now = clk::now();
    double t = sec(now.time_since_epoch()).count();
    self->plot.add_point(t, pwm, rpm, Kp, Ki, Kd);

    update_state_label(self, state);

    if (GTK_IS_PROGRESS_BAR(self->rpm_bar)) {
        double frac = rpm / 5000.0;
        if(frac > 1.0) frac = 1.0;
        if(frac < 0.0) frac = 0.0;
        gtk_progress_bar_set_fraction(
            GTK_PROGRESS_BAR(self->rpm_bar), frac);
    }

    if (GTK_IS_LABEL(self->rpm_label)) {
        m_string text = fmt::format("{:.0f} RPM", rpm);
        gtk_label_set_label(GTK_LABEL(self->rpm_label), text.c_str());
    }

    return G_SOURCE_CONTINUE;
}