#ifndef SYSTEM_INFO_WIDGET_H
#define SYSTEM_INFO_WIDGET_H

#include "light-common.h"
#include <vector>
#include <string>

class SystemInfoWidget {
private:
    // Main containers
    GtkWidget *system_container;
    GtkWidget *stats_vbox;
    GtkWidget *stats_box;
    
    // Drawing canvases for circular progress
    GtkWidget *cpu_canvas;
    GtkWidget *ram_canvas;
    GtkWidget *temp_canvas;
    GtkWidget *disk_canvas;
    
    // Labels for system metrics
    GtkWidget *cpu_label;
    GtkWidget *ram_label;
    GtkWidget *temp_label;
    GtkWidget *disk_label;
    
    // Art container
    GtkWidget *art_vbox;
    
    // System metrics
    double cpu_usage = 0.0;
    double ram_usage = 0.0;
    double temp_usage = 0.0;
    double disk_usage = 0.0;
    double actual_temp = 0.0;
    
    // Hover states for animations
    bool cpu_hovered = false;
    bool ram_hovered = false;
    bool temp_hovered = false;
    bool disk_hovered = false;
    
    // Timers
    guint update_timer_id = 0;
    guint art_timer_id = 0;
    guint animation_timer_id = 0;
    
    // Art rotation
    std::vector<std::string> art_files;
    int current_art_index = 0;
    GtkWidget *art_image;
    
    // Safebooru API
    std::string current_image_url;
    std::string current_image_path;
    
public:
    SystemInfoWidget();
    ~SystemInfoWidget();
    
    GtkWidget* get_widget();
    void refresh();
    void start_art_loading();
    GtkWidget* get_art_image();
    void load_existing_art();
    void refresh_art_image();
    
private:
    void setup_ui();
    void update_system_stats();
    void start_update_timer();
    
    // Art widget methods
    void load_art_files();
    
    // Safebooru API methods
    std::string fetch_random_image_url();
    bool download_image(const std::string& url, const std::string& filepath);
    
    // Drawing methods
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    gboolean on_cpu_draw(GtkWidget *widget, cairo_t *cr);
    gboolean on_ram_draw(GtkWidget *widget, cairo_t *cr);
    gboolean on_temp_draw(GtkWidget *widget, cairo_t *cr);
    gboolean on_disk_draw(GtkWidget *widget, cairo_t *cr);
    
    gboolean draw_circular_progress_with_hover(cairo_t *cr, double percentage, bool is_cpu, bool hovered);
    gboolean draw_circular_progress_for_widget_with_hover(cairo_t *cr, double percentage, GtkWidget *widget, const char* widget_type, bool hovered);
    gboolean draw_circular_progress_for_widget(cairo_t *cr, double percentage, GtkWidget *widget, const char* widget_type);
    gboolean draw_circular_progress(cairo_t *cr, double percentage, bool is_cpu);
    
    // Mouse event handlers
    gboolean on_mouse_enter(GtkWidget *widget, GdkEventCrossing *event, const char* widget_type);
    gboolean on_mouse_leave(GtkWidget *widget, GdkEventCrossing *event, const char* widget_type);
    
    // Animation functions
    gboolean animation_tick();
    void start_animation();
    
    // Static callbacks
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean on_cpu_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean on_ram_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean on_temp_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean on_disk_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    static gboolean on_mouse_enter_static(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);
    static gboolean on_mouse_leave_static(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);
    static gboolean animation_tick_static(gpointer user_data);
};

#endif // SYSTEM_INFO_WIDGET_H
