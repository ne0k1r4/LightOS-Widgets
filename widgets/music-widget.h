#ifndef MUSIC_WIDGET_H
#define MUSIC_WIDGET_H

#include "light-common.h"
#include "playerctl-interface.h"
#include "calendar-widget.h"

class MusicWidget {
private:
    GtkWidget *window;
    GtkWidget *card_container;
    GtkWidget *main_box;
    GtkWidget *root_vbox;
    GtkWidget *album_overlay;
    GtkWidget *album_canvas;
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *progress_bar;
    GtkWidget *current_time_label;
    GtkWidget *total_time_label;
    GtkWidget *play_button;
    GtkWidget *prev_button;
    GtkWidget *next_button;
    
    std::unique_ptr<PlayerctlInterface> media_interface;
    PlayerctlInterface::TrackInfo current_track;
    
    guint update_timer_id = 0;
    guint rotate_timer_id = 0;
    GdkPixbuf *default_album_art = nullptr;
    GdkPixbuf *current_album_art = nullptr;
    GdkPixbuf *disc_pixbuf = nullptr;
    std::string last_art_url = "";
    
    bool is_dragging_progress = false;
    std::string last_title = "";
    std::string last_artist = "";
    double last_duration = 0.0;
    
    // Smooth UI position tracking
    double displayed_position = 0.0;
    std::chrono::steady_clock::time_point last_tick_time = std::chrono::steady_clock::now();
    double last_backend_position = -1.0;
    int stagnant_ticks = 0;
    bool backend_position_suspicious = false;
    bool indeterminate_progress = false;

    // Sizes for disc and inner art
    int disc_size = 64;
    int art_size = 48;

    // Metadata refresh scheduling
    std::chrono::steady_clock::time_point last_metadata_refresh = std::chrono::steady_clock::time_point::min();
    bool has_metadata = false;

    // Rotation state
    double disc_angle_rad = 0.0;
    double art_angle_rad = 0.0;
    double disc_deg_per_sec = 36.0;
    double art_deg_per_sec = 54.0;

    // Calendar widget
    std::unique_ptr<CalendarWidget> calendar_widget;
    
    // Performance optimizations - cached pixbufs and surfaces
    cairo_surface_t *card_surface = nullptr;
    bool surface_needs_update = true;

    // Visibility state
    bool is_visible = false;

    // JSON cache helpers
    static std::string escape_json(const std::string &s);
    void save_cached_metadata(const std::string &title, const std::string &artist);
    void load_cached_metadata();
 
public:
    MusicWidget();
    ~MusicWidget();
    void setup_window();
    void setup_ui();
    void create_default_album_art();
    GdkPixbuf* create_circular_pixbuf(GdkPixbuf* source_pixbuf, int size);
    void load_disc_base_image();
    void load_album_art(const std::string& art_url);
    void update_track_info();
    void apply_styles();
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    static gboolean on_button_press_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static gboolean on_progress_press_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static gboolean on_progress_release_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static gboolean on_progress_motion_static(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
    gboolean on_progress_press(GtkWidget *widget, GdkEventButton *event);
    gboolean on_progress_release(GtkWidget *widget, GdkEventButton *event);
    gboolean on_progress_motion(GtkWidget *widget, GdkEventMotion *event);
    double get_progress_from_position(GtkWidget *widget, double x);
    void update_progress_from_position(GtkWidget *widget, double x);
    static void on_play_pause_static(GtkButton *button, gpointer user_data);
    static void on_next_static(GtkButton *button, gpointer user_data);
    static void on_previous_static(GtkButton *button, gpointer user_data);
    void on_play_pause();
    void on_next();
    void on_previous();
    static gboolean update_timer_callback_static(gpointer user_data);
    void start_update_timer();
    void check_theme_changes();
    void show();
    void hide();
    bool get_visible() const;
    static gboolean on_album_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    gboolean on_album_draw(GtkWidget *widget, cairo_t *cr);
    void update_left_margin_for_center(int content_width);
    static void on_card_size_allocate_static(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);
};

#endif // MUSIC_WIDGET_H


