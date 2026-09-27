#ifndef CALENDAR_WIDGET_H
#define CALENDAR_WIDGET_H

#include "light-common.h"

class CalendarWidget {
private:
    GtkWidget *calendar_container;
    GtkWidget *header_box;
    GtkWidget *month_year_label;
    GtkWidget *grid;
    GtkWidget *prev_month_button;
    GtkWidget *next_month_button;
    
    std::time_t current_time;
    struct tm current_date;
    
    // Cache for avoiding unnecessary rebuilds
    int cached_month = -1;
    int cached_year = -1;
    
    // Special days system
    struct SpecialDay {
        int month;  // 0-11 (January = 0)
        int day;    // 1-31
        std::string icon_path;
        std::string tooltip_text;
    };
    std::vector<SpecialDay> special_days;
    
public:
    CalendarWidget();
    GtkWidget* get_widget();
    void reset_to_today();
    
private:
    void setup_ui();
    void update_calendar();
    void initialize_special_days();
    bool is_special_day(int month, int day);
    std::string get_special_day_icon(int month, int day);
    std::string get_special_day_tooltip(int month, int day);
    void add_icon_to_button(GtkWidget *button, int day);
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data);
    gboolean on_draw(GtkWidget *widget, cairo_t *cr);
    static void on_prev_month_static(GtkButton *button, gpointer user_data);
    static void on_next_month_static(GtkButton *button, gpointer user_data);
    void on_prev_month();
    void on_next_month();
};

#endif // CALENDAR_WIDGET_H

