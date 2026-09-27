#include "calendar-widget.h"

CalendarWidget::CalendarWidget() {
    current_time = std::time(nullptr);
    current_date = *std::localtime(&current_time);
    initialize_special_days();
    setup_ui();
    update_calendar();
}

GtkWidget* CalendarWidget::get_widget() {
    return calendar_container;
}

void CalendarWidget::reset_to_today() {
    current_time = std::time(nullptr);
    current_date = *std::localtime(&current_time);
    // Force cache invalidation
    cached_month = -1;
    cached_year = -1;
    update_calendar();
}

void CalendarWidget::setup_ui() {
    calendar_container = gtk_event_box_new();
    g_signal_connect(calendar_container, "draw", G_CALLBACK(on_draw_static), this);
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(main_vbox, 16);
    gtk_widget_set_margin_end(main_vbox, 16);
    gtk_widget_set_margin_top(main_vbox, 12);
    gtk_widget_set_margin_bottom(main_vbox, 16);
    gtk_container_add(GTK_CONTAINER(calendar_container), main_vbox);
    
    header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_bottom(header_box, 12);
    
    prev_month_button = gtk_button_new();
    GtkWidget *prev_icon = gtk_label_new("‹");
    gtk_widget_set_name(prev_icon, "nav-icon");
    gtk_container_add(GTK_CONTAINER(prev_month_button), prev_icon);
    gtk_button_set_relief(GTK_BUTTON(prev_month_button), GTK_RELIEF_NONE);
    gtk_widget_set_name(prev_month_button, "nav-button");
    g_signal_connect(prev_month_button, "clicked", G_CALLBACK(on_prev_month_static), this);
    gtk_box_pack_start(GTK_BOX(header_box), prev_month_button, FALSE, FALSE, 0);
    
    month_year_label = gtk_label_new("");
    gtk_widget_set_name(month_year_label, "month-year-label");
    gtk_widget_set_hexpand(month_year_label, TRUE);
    gtk_widget_set_halign(month_year_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header_box), month_year_label, TRUE, TRUE, 0);
    
    next_month_button = gtk_button_new();
    GtkWidget *next_icon = gtk_label_new("›");
    gtk_widget_set_name(next_icon, "nav-icon");
    gtk_container_add(GTK_CONTAINER(next_month_button), next_icon);
    gtk_button_set_relief(GTK_BUTTON(next_month_button), GTK_RELIEF_NONE);
    gtk_widget_set_name(next_month_button, "nav-button");
    g_signal_connect(next_month_button, "clicked", G_CALLBACK(on_next_month_static), this);
    gtk_box_pack_start(GTK_BOX(header_box), next_month_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), header_box, FALSE, FALSE, 0);
    
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), grid, TRUE, TRUE, 0);
}

void CalendarWidget::update_calendar() {
    // Skip rebuild if month/year unchanged
    if (cached_month == current_date.tm_mon && cached_year == current_date.tm_year) {
        return;
    }
    cached_month = current_date.tm_mon;
    cached_year = current_date.tm_year;
    
    // Clear existing grid children
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Update month/year label
    static const char* months[] = {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "%s %d", months[current_date.tm_mon], current_date.tm_year + 1900);
    gtk_label_set_text(GTK_LABEL(month_year_label), date_str);
    
    // Add day headers
    static const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    for (int i = 0; i < 7; i++) {
        GtkWidget *day_header = gtk_label_new(days[i]);
        gtk_widget_set_name(day_header, "day-header");
        gtk_widget_set_size_request(day_header, 32, 20);
        gtk_grid_attach(GTK_GRID(grid), day_header, i, 0, 1, 1);
    }
    
    // Calculate first day of month
    struct tm first_day = current_date;
    first_day.tm_mday = 1;
    mktime(&first_day);
    
    // Get current date for highlighting
    std::time_t now = std::time(nullptr);
    struct tm today = *std::localtime(&now);
    
    // Calculate days in current month properly
    int days_in_month;
    if (current_date.tm_mon == 0 || current_date.tm_mon == 2 || current_date.tm_mon == 4 || 
        current_date.tm_mon == 6 || current_date.tm_mon == 7 || current_date.tm_mon == 9 || current_date.tm_mon == 11) {
        // January, March, May, July, August, October, December - 31 days
        days_in_month = 31;
    } else if (current_date.tm_mon == 3 || current_date.tm_mon == 5 || current_date.tm_mon == 8 || current_date.tm_mon == 10) {
        // April, June, September, November - 30 days
        days_in_month = 30;
    } else {
        // February - check for leap year
        int year = current_date.tm_year + 1900;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            days_in_month = 29; // Leap year
        } else {
            days_in_month = 28; // Regular year
        }
    }
    
    // Fill in the calendar
    int row = 1;
    int col = first_day.tm_wday;
    
    // Previous month's trailing days
    if (col > 0) {
        struct tm prev_month = current_date;
        prev_month.tm_mon--;
        if (prev_month.tm_mon < 0) {
            prev_month.tm_mon = 11;
            prev_month.tm_year--;
        }
        prev_month.tm_mday = 1;
        mktime(&prev_month);
        
        struct tm prev_last = prev_month;
        prev_last.tm_mon++;
        if (prev_last.tm_mon > 11) {
            prev_last.tm_mon = 0;
            prev_last.tm_year++;
        }
        prev_last.tm_mday = 0;
        mktime(&prev_last);
        
        int prev_days = prev_last.tm_mday;
        for (int i = col - 1; i >= 0; i--) {
            int day = prev_days - i;
            GtkWidget *day_button = gtk_button_new_with_label(std::to_string(day).c_str());
            gtk_widget_set_name(day_button, "day-other");
            gtk_widget_set_size_request(day_button, 32, 28);
            gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
            gtk_grid_attach(GTK_GRID(grid), day_button, col - 1 - i, row, 1, 1);
        }
    }
    
    // Current month's days - EXACTLY days_in_month days, no more!
    for (int day = 1; day <= days_in_month; day++) {
        if (col >= 7) {
            col = 0;
            row++;
        }
        
        GtkWidget *day_button = gtk_button_new_with_label(std::to_string(day).c_str());
        gtk_widget_set_size_request(day_button, 32, 28);
        gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
        
        bool is_today = (current_date.tm_year == today.tm_year && 
                       current_date.tm_mon == today.tm_mon && 
                       day == today.tm_mday);
        
        if (is_today) {
            gtk_widget_set_name(day_button, "day-today");
        } else {
            gtk_widget_set_name(day_button, "day-current");
        }
        
        // Add icon for special days
        add_icon_to_button(day_button, day);
        
        gtk_grid_attach(GTK_GRID(grid), day_button, col, row, 1, 1);
        col++;
    }
    
    // Next month's leading days
    int next_month_day = 1;
    while (col < 7 && row <= 6) {
        GtkWidget *day_button = gtk_button_new_with_label(std::to_string(next_month_day).c_str());
        gtk_widget_set_name(day_button, "day-other");
        gtk_widget_set_size_request(day_button, 32, 28);
        gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
        gtk_grid_attach(GTK_GRID(grid), day_button, col, row, 1, 1);
        next_month_day++;
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
    
    gtk_widget_show_all(calendar_container);
}

gboolean CalendarWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean CalendarWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double width = allocation.width;
    double height = allocation.height;
    double radius = 14.0;
    
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    
    // Drop shadow
    cairo_save(cr);
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius+2, radius+2, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius + 2, radius + 2, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius + 2, height - radius + 2, radius, 0, M_PI / 2);
    cairo_arc(cr, radius + 2, height - radius + 2, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.18);
    cairo_fill(cr);
    cairo_restore(cr);

    // Card background
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);

    // Theme gradient
    cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, width, height);
    if (g_use_hoc_theme) {
        cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.694, 0.788, 0.925, 0.60);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.439, 0.467, 0.741, 0.45);
    } else {
        cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.933, 0.698, 0.812, 0.90);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.94, 0.98, 0.16);
    }
    cairo_set_source(cr, gradient);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, 1, 1, 1, 0.65);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    cairo_pattern_destroy(gradient);
    
    return FALSE;
}

void CalendarWidget::on_prev_month_static(GtkButton *button, gpointer user_data) {
    CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
    self->on_prev_month();
}

void CalendarWidget::on_next_month_static(GtkButton *button, gpointer user_data) {
    CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
    self->on_next_month();
}

void CalendarWidget::on_prev_month() {
    current_date.tm_mon--;
    if (current_date.tm_mon < 0) {
        current_date.tm_mon = 11;
        current_date.tm_year--;
    }
    mktime(&current_date);
    update_calendar();
}

void CalendarWidget::on_next_month() {
    current_date.tm_mon++;
    if (current_date.tm_mon > 11) {
        current_date.tm_mon = 0;
        current_date.tm_year++;
    }
    mktime(&current_date);
    update_calendar();
}

void CalendarWidget::initialize_special_days() {
    // LightOS ships without franchise-specific calendar entries.
}

bool CalendarWidget::is_special_day(int month, int day) {
    for (const auto& special_day : special_days) {
        if (special_day.month == month && special_day.day == day) {
            return true;
        }
    }
    return false;
}

std::string CalendarWidget::get_special_day_icon(int month, int day) {
    for (const auto& special_day : special_days) {
        if (special_day.month == month && special_day.day == day) {
            return special_day.icon_path;
        }
    }
    return "";
}

std::string CalendarWidget::get_special_day_tooltip(int month, int day) {
    for (const auto& special_day : special_days) {
        if (special_day.month == month && special_day.day == day) {
            return special_day.tooltip_text;
        }
    }
    return "";
}

void CalendarWidget::add_icon_to_button(GtkWidget *button, int day) {
    // Only add icon for current month special days
    if (is_special_day(current_date.tm_mon, day)) {
        std::string icon_path = get_special_day_icon(current_date.tm_mon, day);
        std::string tooltip_text = get_special_day_tooltip(current_date.tm_mon, day);
        
        if (!icon_path.empty()) {
            // Create small icon (16x16 pixels) - actually scale the image
            GtkWidget *icon = gtk_image_new();
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(icon_path.c_str(), NULL);
            if (pixbuf) {
                // Scale the image to 16x16
                GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 16, 16, GDK_INTERP_BILINEAR);
                gtk_image_set_from_pixbuf(GTK_IMAGE(icon), scaled_pixbuf);
                g_object_unref(scaled_pixbuf);
                g_object_unref(pixbuf);
                
                // Position icon below the day number
                gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
                gtk_widget_set_valign(icon, GTK_ALIGN_END);
                gtk_widget_set_margin_bottom(icon, 1);
                
                // Add icon below the day number using a vertical box container
                GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
                GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
                
                gtk_container_remove(GTK_CONTAINER(button), label);
                gtk_container_add(GTK_CONTAINER(button), button_box);
                
                // Recreate the label with the day number directly
                GtkWidget *new_label = gtk_label_new(std::to_string(day).c_str());
                gtk_widget_set_halign(new_label, GTK_ALIGN_CENTER);
                gtk_widget_set_valign(new_label, GTK_ALIGN_CENTER);
                
                gtk_box_pack_start(GTK_BOX(button_box), new_label, TRUE, TRUE, 0);
                gtk_box_pack_end(GTK_BOX(button_box), icon, FALSE, FALSE, 0);
                
                // Add tooltip to the button
                if (!tooltip_text.empty()) {
                    gtk_widget_set_tooltip_text(button, tooltip_text.c_str());
                }
            }
        }
    }
}

