#ifndef VOLUME_WIDGET_H
#define VOLUME_WIDGET_H

#include "light-common.h"

class VolumeWidget {
private:
    GtkWidget *window;
    GtkWidget *root_vbox;
    GtkWidget *card_container;
    GtkWidget *main_box;
    GtkWidget *volume_icon;
    GtkWidget *volume_label;
    GtkWidget *volume_value_label;
    GtkWidget *volume_progress;
    
    int current_volume;
    bool is_muted = false;
    bool is_visible = false;
    guint hide_timer_id = 0;
    
public:
    VolumeWidget();
    ~VolumeWidget();
    
    void show();
    void hide();
    bool get_visible();
    void update_volume(int volume);
    void increase_volume();
    void decrease_volume();
    void toggle_mute();
    GtkWidget* get_widget();
    
private:
    void setup_window();
    void setup_ui();
    void apply_styles();
    void get_current_volume();
    void set_volume(int volume);
    void update_display();
    void center_widget_vertically();
    int safe_string_to_int(const std::string& str, int fallback);
    
    // Static callbacks
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean hide_timeout_static(gpointer user_data);
    
    // Instance callbacks
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    gboolean hide_timeout();
};

#endif // VOLUME_WIDGET_H

