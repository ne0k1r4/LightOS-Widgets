#ifndef RECENT_APPS_WIDGET_H
#define RECENT_APPS_WIDGET_H

#include "light-common.h"

class RecentAppsWidget {
private:
    GtkWidget *apps_container;
    GtkWidget *header_label;
    GtkWidget *apps_grid;
    
    struct AppInfo {
        std::string name;
        int usage_count;
        std::string icon_path;
    };
    
    std::vector<AppInfo> recent_apps;
    
    // Cache for desktop file information to avoid repeated file system access
    static std::unordered_map<std::string, std::string> icon_cache;
    static bool cache_initialized;
    
    // Hover effects are handled by CSS - no custom state tracking needed
    
public:
    RecentAppsWidget();
    GtkWidget* get_widget();
    void refresh();
    static void initialize_icon_cache();
    
private:
    void setup_ui();
    void load_recent_apps();
    std::string find_app_icon(const std::string& app_name);
    std::string get_icon_from_desktop_file(const std::string& app_name);
    std::string find_icon_with_gtk_theme(const std::string& icon_name);
    void update_display();
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    static std::string find_desktop_file_name(const std::string& app_name);
    static void on_app_clicked_static(GtkButton *button, gpointer user_data);
    
    // Hover effects are handled by CSS - no custom handlers needed
};

#endif // RECENT_APPS_WIDGET_H
