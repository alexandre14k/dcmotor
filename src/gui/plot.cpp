#include "plot.hpp"

MotorPlot::MotorPlot() {}
MotorPlot::~MotorPlot() {}

void MotorPlot::ensure_init() {
    if (!area) {
        area = gtk_drawing_area_new();
        gtk_widget_set_size_request(area, 400, -1);
        gtk_widget_set_vexpand(area, TRUE);
        gtk_widget_set_valign(area, GTK_ALIGN_FILL);
        gtk_widget_set_hexpand(area, TRUE);
        gtk_widget_set_halign(area, GTK_ALIGN_FILL);
        gtk_drawing_area_set_draw_func(
            GTK_DRAWING_AREA(area), on_draw, this, NULL);
    }
}

GtkWidget* MotorPlot::get_widget() {
    ensure_init();
    return area;
}

void MotorPlot::set_log_view(bool en) {
    log_view.store(en);
    ensure_init();
    if(GTK_IS_WIDGET(area))
        gtk_widget_queue_draw(area);
}

void MotorPlot::set_tracking(bool en, double rpm) {
    track_enabled.store(en);
    track_rpm.store(rpm);
    ensure_init();
    if(GTK_IS_WIDGET(area))
        gtk_widget_queue_draw(area);
}

void MotorPlot::adjust_time_window(double delta) {
    double new_window = time_window.load() + delta;
    if (new_window < 2.0) new_window = 2.0;
    if (new_window > 60.0) new_window = 60.0;
    time_window.store(new_window);
    ensure_init();
    if(GTK_IS_WIDGET(area))
        gtk_widget_queue_draw(area);
}

void MotorPlot::add_point(double t, double pwm, double rpm,
                          double Kp, double Ki, double Kd) {
    data.push_back({t, pwm, rpm, Kp, Ki, Kd});
    double tw = time_window.load();
    while(data.size() > 1 &&
          data.back().t - data.front().t > tw) {
        data.erase(data.begin());
    }
    ensure_init();
    if(GTK_IS_WIDGET(area))
        gtk_widget_queue_draw(area);
}

void MotorPlot::draw_background_and_grid(cairo_t* cr, int width,
    int height, double pad_l, double pad_r, double pad_t,
    double pad_b, bool log_view, const PlotColors& colors) {

    cairo_set_source_rgb(cr, colors.bg_r, colors.bg_g, colors.bg_b);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, colors.grid_r, colors.grid_g, colors.grid_b);
    cairo_set_line_width(cr, 1.0);
    double plot_w = width - pad_l - pad_r;
    double plot_h = height - pad_t - pad_b;

    for(int i=0; i<=10; ++i) {
        double x = pad_l + (plot_w / 10) * i;
        cairo_move_to(cr, x, pad_t);
        cairo_line_to(cr, x, height - pad_b);
    }

    if (log_view) {
        double max_val = 5000.0;
        double log_max = log10(max_val + 1.0);
        if (log_max < 1e-6) log_max = 1.0;

        for (int d = 0; d <= 3; ++d) {
            for (int m = 1; m <= 9; ++m) {
                double val = m * std::pow(10, d);
                if (val > max_val) break;
                double y = pad_t + plot_h -
                    (log10(val + 1.0) / log_max) * plot_h;
                cairo_move_to(cr, pad_l, y);
                cairo_line_to(cr, width - pad_r, y);
            }
        }
    } else {
        for(int i=0; i<=5; ++i) {
            double y = pad_t + (plot_h / 5) * i;
            cairo_move_to(cr, pad_l, y);
            cairo_line_to(cr, width - pad_r, y);
        }
    }
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, colors.text_r, colors.text_g, colors.text_b);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, pad_l, pad_t);
    cairo_line_to(cr, pad_l, height - pad_b);
    cairo_stroke(cr);
}

void MotorPlot::draw_title_and_legend(cairo_t* cr, double pad_l,
    double plot_w, double pad_t, bool log_view, const PlotColors& colors, bool track_en) {

    cairo_set_source_rgb(cr, colors.text_r, colors.text_g, colors.text_b);
    cairo_select_font_face(cr, "Sans",
        CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);

    const char* title = log_view ? "Motor Data Plot (Log View)" : "Motor Data Plot";
    cairo_text_extents_t ext;

    // Align title to the left
    cairo_move_to(cr, pad_l, pad_t - 30);
    cairo_show_text(cr, title);

    cairo_set_font_size(cr, 12.0);

    struct LegendItem {
        const char* text;
        double r, g, b;
    };

    std::vector<LegendItem> items = {
        {"PWM (%)", colors.pwm_r, colors.pwm_g, colors.pwm_b},
        {"RPM", colors.rpm_r, colors.rpm_g, colors.rpm_b},
        {"Kp", colors.kp_r, colors.kp_g, colors.kp_b},
        {"Ki", colors.ki_r, colors.ki_g, colors.ki_b},
        {"Kd", colors.kd_r, colors.kd_g, colors.kd_b}
    };

    if (track_en) {
        items.push_back({"Target RPM", 0.2, 0.8, 0.2}); // Lime
    }

    // Align legend to the left, starting at pad_l
    double x = pad_l;
    for (const auto& item : items) {
        cairo_set_source_rgb(cr, item.r, item.g, item.b);
        cairo_rectangle(cr, x, pad_t - 15, 10, 10);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, colors.text_r, colors.text_g, colors.text_b);
        cairo_move_to(cr, x + 15, pad_t - 6);
        cairo_show_text(cr, item.text);

        cairo_text_extents(cr, item.text, &ext);
        x += 10 + 5 + ext.width + 20;
    }
}

void MotorPlot::draw_axis_labels(cairo_t* cr, int height, double pad_l,
    double plot_w, double pad_t, double pad_b, bool log_view, double max_pid, const PlotColors& colors) {

    cairo_set_source_rgb(cr, colors.text_r, colors.text_g, colors.text_b);
    cairo_set_font_size(cr, 10.0);

    cairo_text_extents_t ext;
    m_string max_lbl_str = log_view ?
        "Log(5000/100/PID)" :
        fmt::format("5000/100/{:.1f}", max_pid);
    const char* max_lbl = max_lbl_str.c_str();

    cairo_text_extents(cr, max_lbl, &ext);
    // Aligned right-justified to the left of the Y-axis
    cairo_move_to(cr, pad_l - ext.width - 5, pad_t + 5);
    cairo_show_text(cr, max_lbl);

    const char* min_lbl = "0/0/0";
    cairo_text_extents(cr, min_lbl, &ext);
    cairo_move_to(cr, pad_l - ext.width - 5, height - pad_b + 5);
    cairo_show_text(cr, min_lbl);

    const char* time_lbl = "Time (s)";
    cairo_text_extents(cr, time_lbl, &ext);
    double time_x = pad_l + (plot_w - ext.width) / 2.0;
    cairo_move_to(cr, time_x, height - pad_b + 15);
    cairo_show_text(cr, time_lbl);
}

void MotorPlot::draw_plot_line(cairo_t* cr, const std::vector<PlotPoint>& data,
    double min_t, double range_t, double plot_w, double plot_h,
    double pad_l, double pad_t,
    std::function<double(const PlotPoint&)> getter,
    double max_val, double r, double g, double b, bool log_view) {

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 2.0);

    double log_max = log10(max_val + 1.0);
    if (log_max < 1e-6) log_max = 1.0;

    for (size_t i = 0; i < data.size(); ++i) {
        double x = pad_l + (data[i].t - min_t) / range_t * plot_w;
        double val = getter(data[i]);
        double safe_val = std::max(0.0, val);
        double y;
        if (log_view) {
            y = pad_t + plot_h -
                (log10(safe_val + 1.0) / log_max) * plot_h;
        } else {
            y = pad_t + plot_h - (safe_val / max_val) * plot_h;
        }
        if (i == 0) cairo_move_to(cr, x, y);
        else cairo_line_to(cr, x, y);
    }
    cairo_stroke(cr);
}

void MotorPlot::on_draw(GtkDrawingArea* area, cairo_t* cr,
    int width, int height, gpointer data) {

    MotorPlot* self = static_cast<MotorPlot*>(data);
    if (!self || !cr) return;

    // Theme detection using gtk_widget_get_color (GTK4 non-deprecated)
    GdkRGBA fg_color;
    gtk_widget_get_color(GTK_WIDGET(area), &fg_color);
    bool is_dark = (fg_color.red + fg_color.green + fg_color.blue) / 3.0 > 0.5;

    PlotColors colors;
    if (is_dark) {
        colors = { 0.12, 0.12, 0.12,  // bg
                   0.3, 0.3, 0.3,     // grid
                   0.9, 0.9, 0.9,     // text
                   0.2, 0.6, 1.0,     // pwm (blue)
                   1.0, 0.3, 0.3,     // rpm (red)
                   0.0, 1.0, 1.0,     // kp (cyan)
                   1.0, 0.0, 1.0,     // ki (magenta)
                   1.0, 0.84, 0.0 };  // kd (gold)
    } else {
        colors = { 1.0, 1.0, 1.0,     // bg
                   0.8, 0.8, 0.8,     // grid
                   0.0, 0.0, 0.0,     // text
                   0.0, 0.0, 1.0,     // pwm
                   1.0, 0.0, 0.0,     // rpm
                   0.0, 1.0, 1.0,     // kp
                   1.0, 0.0, 1.0,     // ki
                   1.0, 0.84, 0.0 };  // kd
    }

    bool log_view = self->log_view.load();
    bool track_en = self->track_enabled.load();
    double track_rpm_val = self->track_rpm.load();

    double max_pid = 1.0;
    for (const auto& p : self->data) {
        if (p.Kp > max_pid) max_pid = p.Kp;
        if (p.Ki > max_pid) max_pid = p.Ki;
        if (p.Kd > max_pid) max_pid = p.Kd;
    }

    // Dynamic left padding based on Y-axis label width to prevent truncation
    m_string max_lbl_str = log_view ? "Log(5000/100/PID)" : fmt::format("5000/100/{:.1f}", max_pid);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, max_lbl_str.c_str(), &ext);

    double pad_l = std::max(40.0, ext.width + 15.0);
    double pad_r = 20.0;
    double pad_t = 50.0;
    double pad_b = 30.0;
    double plot_w = width - pad_l - pad_r;
    double plot_h = height - pad_t - pad_b;

    // Prevent negative dimensions if window is extremely small
    if (plot_w < 10) plot_w = 10;
    if (plot_h < 10) plot_h = 10;

    draw_background_and_grid(cr, width, height, pad_l, pad_r,
        pad_t, pad_b, log_view, colors);
    draw_title_and_legend(cr, pad_l, plot_w, pad_t, log_view, colors, track_en);
    draw_axis_labels(cr, height, pad_l, plot_w, pad_t, pad_b, log_view,
        max_pid, colors);

    if (track_en) {
        // Draw dotted line for target RPM
        cairo_set_source_rgb(cr, 0.2, 0.8, 0.2); // Lime
        cairo_set_line_width(cr, 1.5);
        double dashes[] = {5.0, 5.0};
        cairo_set_dash(cr, dashes, 2, 0);

        double y;
        if (log_view) {
            double log_max = log10(5000.0 + 1.0);
            y = pad_t + plot_h - (log10(std::max(0.0, track_rpm_val) + 1.0) / log_max) * plot_h;
        } else {
            y = pad_t + plot_h - (track_rpm_val / 5000.0) * plot_h;
        }

        cairo_move_to(cr, pad_l, y);
        cairo_line_to(cr, width - pad_r, y);
        cairo_stroke(cr);
        cairo_set_dash(cr, nullptr, 0, 0);
    }

    if (self->data.empty()) return;

    double min_t = self->data.front().t;
    double max_t = self->data.back().t;
    double tw = self->time_window.load();
    if (max_t - min_t < tw) max_t = min_t + tw;
    double range_t = max_t - min_t;
    if (range_t < 1e-6) range_t = 1.0; // Prevent division by zero

    draw_plot_line(cr, self->data, min_t, range_t, plot_w, plot_h,
        pad_l, pad_t, [](const PlotPoint& p){ return p.pwm; },
        100.0, colors.pwm_r, colors.pwm_g, colors.pwm_b, log_view);
    draw_plot_line(cr, self->data, min_t, range_t, plot_w, plot_h,
        pad_l, pad_t, [](const PlotPoint& p){ return p.rpm; },
        5000.0, colors.rpm_r, colors.rpm_g, colors.rpm_b, log_view);
    draw_plot_line(cr, self->data, min_t, range_t, plot_w, plot_h,
        pad_l, pad_t, [](const PlotPoint& p){ return p.Kp; },
        max_pid, colors.kp_r, colors.kp_g, colors.kp_b, log_view);
    draw_plot_line(cr, self->data, min_t, range_t, plot_w, plot_h,
        pad_l, pad_t, [](const PlotPoint& p){ return p.Ki; },
        max_pid, colors.ki_r, colors.ki_g, colors.ki_b, log_view);
    draw_plot_line(cr, self->data, min_t, range_t, plot_w, plot_h,
        pad_l, pad_t, [](const PlotPoint& p){ return p.Kd; },
        max_pid, colors.kd_r, colors.kd_g, colors.kd_b, log_view);
}