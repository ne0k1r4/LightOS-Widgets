#ifndef BRIGHTNESS_WIDGET_H
#define BRIGHTNESS_WIDGET_H

#include "light-common.h"
#include <string>

class BrightnessWidget {
private:
    GtkWidget *window;
    GtkWidget *root_vbox;
    GtkWidget *card_container;
    GtkWidget *main_box;
    GtkWidget *brightness_icon;
    GtkWidget *brightness_label;
    GtkWidget *brightness_value_label;
    GtkWidget *brightness_progress;
    
    int current_brightness;
    bool is_visible = false;
    guint hide_timer_id = 0;
    
public:
    BrightnessWidget();
    ~BrightnessWidget();
    
    void show();
    void hide();
    bool get_visible();
    void update_brightness(int brightness);
    void increase_brightness();
    void decrease_brightness();
    GtkWidget* get_widget();
    
private:
    void setup_window();
    void setup_ui();
    void apply_styles();
    void get_current_brightness();
    void set_brightness(int brightness);
    void update_display();
    void center_widget_vertically();
    int safe_string_to_int(const std::string& str, int fallback);
    void handle_progress_interaction(double x, int width);
    
    // Static callbacks
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean hide_timeout_static(gpointer user_data);
    static gboolean on_progress_button_press_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static gboolean on_progress_button_release_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static gboolean on_progress_motion_notify_static(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
    
    // Instance callbacks
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    gboolean hide_timeout();
    gboolean on_progress_button_press(GtkWidget *widget, GdkEventButton *event);
    gboolean on_progress_button_release(GtkWidget *widget, GdkEventButton *event);
    gboolean on_progress_motion_notify(GtkWidget *widget, GdkEventMotion *event);
};

#endif // BRIGHTNESS_WIDGET_H