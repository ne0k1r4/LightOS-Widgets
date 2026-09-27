#ifndef HOST_WIDGET_H
#define HOST_WIDGET_H

#include "light-common.h"

class SystemInfoWidget; // Forward declaration

class HostWidget {
private:
    GtkWidget *host_container;
    GtkWidget *user_info_box;
    GtkWidget *user_icon;
    GtkWidget *username_label;
    GtkWidget *actions_box;
    GtkWidget *refresh_button;
    GtkWidget *record_button;
    GtkWidget *screenshot_button;
    GtkWidget *powermenu_button;
    
    std::string current_username;
    SystemInfoWidget *system_info_widget;
    
public:
    HostWidget();
    GtkWidget* get_widget();
    void update_username();
    void refresh_icon();
    void set_system_info_widget(SystemInfoWidget *widget);
    
private:
    void setup_ui();
    void apply_styles();
    void load_user_icon();
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    static gboolean on_user_icon_clicked_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static void on_refresh_clicked_static(GtkButton *button, gpointer user_data);
    static void on_record_clicked_static(GtkButton *button, gpointer user_data);
    static void on_screenshot_clicked_static(GtkButton *button, gpointer user_data);
    static void on_powermenu_clicked_static(GtkButton *button, gpointer user_data);
    void on_user_icon_clicked();
    void on_refresh_clicked();
    void on_record_clicked();
    void on_screenshot_clicked();
    void on_powermenu_clicked();
    void hide_stats_widget();
    void show_file_selector();
    void copy_icon_file(const std::string& source_path);
};

#endif // HOST_WIDGET_H
