#include "brightness-widget.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <gtk-layer-shell.h>
#include <string>
#include <stdexcept>

BrightnessWidget::BrightnessWidget() : current_brightness(50) {
    // Detect theme once at startup
    detect_global_theme();
    
    setup_window();
    setup_ui();
    apply_styles();
    
    get_current_brightness();
    update_display();
}

BrightnessWidget::~BrightnessWidget() {
    if (hide_timer_id != 0) {
        g_source_remove(hide_timer_id);
    }
    if (window) {
        gtk_widget_destroy(window);
    }
}

void BrightnessWidget::setup_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Brightness Widget");
    // Smaller, more compact design - matching volume widget
    gtk_window_set_default_size(GTK_WINDOW(window), 70, 220);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);

    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, FALSE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 20);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
    
    // Center the widget vertically
    center_widget_vertically();

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(window, visual);
    }

    gtk_widget_set_app_paintable(window, TRUE);
}

void BrightnessWidget::setup_ui() {
    // Root container
    root_vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root_vbox);
    
    // Card container with custom drawing
    card_container = gtk_event_box_new();
    gtk_widget_set_name(card_container, "brightness-card");
    g_signal_connect(card_container, "draw", G_CALLBACK(on_draw_static), this);
    gtk_box_pack_start(GTK_BOX(root_vbox), card_container, TRUE, TRUE, 0);
    
    // Main content box - reduced padding for smaller size
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_widget_set_margin_top(main_box, 14);
    gtk_widget_set_margin_bottom(main_box, 14);
    gtk_container_add(GTK_CONTAINER(card_container), main_box);
    
    // Content container
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
    
    // Title and value container
    GtkWidget *title_value_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_set_halign(title_value_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), title_value_box, FALSE, FALSE, 0);
    
    // Brightness title
    brightness_label = gtk_label_new("Brightness");
    gtk_widget_set_name(brightness_label, "brightness-title");
    gtk_widget_set_halign(brightness_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(title_value_box), brightness_label, FALSE, FALSE, 0);
    
    // Brightness value
    brightness_value_label = gtk_label_new("50%");
    gtk_widget_set_name(brightness_value_label, "brightness-value");
    gtk_widget_set_halign(brightness_value_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(title_value_box), brightness_value_label, FALSE, FALSE, 0);
    
    // Progress section
    GtkWidget *progress_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(progress_section, 8);
    
    GtkWidget *progress_container = gtk_event_box_new();
    gtk_widget_set_size_request(progress_container, 16, 130);
    
    brightness_progress = gtk_progress_bar_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(brightness_progress), GTK_ORIENTATION_VERTICAL);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(brightness_progress), 0.0);
    gtk_widget_set_size_request(brightness_progress, 6, 130);
    gtk_widget_set_name(brightness_progress, "brightness-progress");
    gtk_widget_set_halign(brightness_progress, GTK_ALIGN_CENTER);
    
    gtk_container_add(GTK_CONTAINER(progress_container), brightness_progress);
    gtk_box_pack_start(GTK_BOX(progress_section), progress_container, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), progress_section, FALSE, FALSE, 0);
    
    // Brightness icon
    brightness_icon = gtk_image_new_from_icon_name("light-brightness", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_size_request(brightness_icon, 24, 24);
    gtk_widget_set_halign(brightness_icon, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(brightness_icon, GTK_ALIGN_END);
    gtk_widget_set_name(brightness_icon, "brightness-icon");
    gtk_widget_set_margin_top(brightness_icon, 8);
    gtk_box_pack_start(GTK_BOX(main_box), brightness_icon, FALSE, FALSE, 0);
}

void BrightnessWidget::apply_styles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    std::string css;
    if (g_use_hoc_theme) {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #brightness-title { font-family: LightOSNew12; font-size: 11px; font-weight: 600; color: #ffffff; margin-bottom: 2px; text-align: center; }
            #brightness-value { font-family: LightOSNew12; font-size: 12px; font-weight: 400; color: #ffffff; text-align: center; opacity: 0.9; }
            #brightness-progress { background-color: rgba(255,255,255,0.3); border-radius: 3px; cursor: pointer; }
            #brightness-progress progress { background: linear-gradient(180deg, #7077bd 0%, #b1c9ec 100%); border-radius: 3px; border: none; }
            #brightness-icon { color: #ffffff; -gtk-icon-size: 22px; opacity: 0.95; }
        )";
    } else {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #brightness-title { font-family: LightOSNew12; font-size: 11px; font-weight: 600; color: #ffffff; margin-bottom: 2px; text-align: center; }
            #brightness-value { font-family: LightOSNew12; font-size: 12px; font-weight: 400; color: #ffffff; text-align: center; opacity: 0.9; }
            #brightness-progress { background-color: rgba(255,255,255,0.3); border-radius: 3px; cursor: pointer; }
            #brightness-progress progress { background: linear-gradient(180deg, #ff6bb3 0%, #ffc4e1 100%); border-radius: 3px; border: none; }
            #brightness-icon { color: #ffffff; -gtk-icon-size: 22px; opacity: 0.95; }
        )";
    }
    
    gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    g_object_unref(provider);
}

int BrightnessWidget::safe_string_to_int(const std::string& str, int fallback) {
    try {
        // Remove any whitespace and newlines
        std::string cleaned = str;
        cleaned.erase(cleaned.find_last_not_of(" \n\r\t") + 1);
        cleaned.erase(0, cleaned.find_first_not_of(" \n\r\t"));
        
        if (cleaned.empty()) {
            return fallback;
        }
        
        return std::stoi(cleaned);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing number from string '" << str << "': " << e.what() << std::endl;
        return fallback;
    }
}

void BrightnessWidget::get_current_brightness() {
    int max_brightness = 100; // fallback
    int raw_brightness = 50;  // fallback
    
    // Get max brightness first
    FILE *max_pipe = popen("brightnessctl max 2>/dev/null", "r");
    if (max_pipe) {
        char max_buffer[128];
        if (fgets(max_buffer, sizeof(max_buffer), max_pipe)) {
            max_brightness = safe_string_to_int(std::string(max_buffer), 100);
            if (max_brightness <= 0) max_brightness = 100; // Sanity check
        }
        pclose(max_pipe);
    }
    
    // Get current brightness
    FILE *pipe = popen("brightnessctl get 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            raw_brightness = safe_string_to_int(std::string(buffer), max_brightness / 2);
            if (raw_brightness < 0) raw_brightness = 0; // Sanity check
        }
        pclose(pipe);
    }
    
    // Convert to percentage with proper bounds checking
    if (max_brightness > 0) {
        current_brightness = (int)((double)raw_brightness * 100.0 / (double)max_brightness + 0.5);
    } else {
        current_brightness = 50; // fallback
    }
    
    // Ensure bounds
    if (current_brightness > 100) current_brightness = 100;
    if (current_brightness < 0) current_brightness = 0;
    
    std::cout << "Brightness: raw=" << raw_brightness << ", max=" << max_brightness << ", percentage=" << current_brightness << std::endl;
}

void BrightnessWidget::set_brightness(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    
    std::stringstream cmd;
    cmd << "brightnessctl set " << brightness << "% 2>/dev/null";
    
    int result = system(cmd.str().c_str());
    if (result == 0) {
        current_brightness = brightness;
        std::cout << "Set brightness to " << brightness << "%" << std::endl;
    } else {
        std::cerr << "Failed to set brightness to " << brightness << "%" << std::endl;
        // Don't update current_brightness if the command failed
        // Instead, refresh from system
        get_current_brightness();
    }
    
    update_display();
}

void BrightnessWidget::update_display() {
    // Update value label
    std::string value_text = std::to_string(current_brightness) + "%";
    gtk_label_set_text(GTK_LABEL(brightness_value_label), value_text.c_str());
    
    // Update progress bar
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(brightness_progress), current_brightness / 100.0);
}

void BrightnessWidget::show() {
    if (!is_visible) {
        is_visible = true;
        // Refresh current brightness when showing
        get_current_brightness();
        // Refresh theme detection when showing the widget
        refresh_global_theme();
        // Reapply styles with new theme
        apply_styles();
        // Force redraw of all widgets to reflect theme changes
        gtk_widget_queue_draw(card_container);
        gtk_widget_show_all(window);
        
        // Auto-hide after 2 seconds
        if (hide_timer_id != 0) {
            g_source_remove(hide_timer_id);
        }
        hide_timer_id = g_timeout_add_seconds(2, hide_timeout_static, this);
    }
}

void BrightnessWidget::hide() {
    if (is_visible) {
        is_visible = false;
        if (hide_timer_id != 0) {
            g_source_remove(hide_timer_id);
            hide_timer_id = 0;
        }
        gtk_widget_hide(window);
    }
}

bool BrightnessWidget::get_visible() {
    return is_visible;
}

void BrightnessWidget::increase_brightness() {
    // Always get fresh brightness value before modifying
    get_current_brightness();
    
    int new_brightness = current_brightness + 5;
    if (new_brightness > 100) new_brightness = 100;
    
    set_brightness(new_brightness);
    show();
}

void BrightnessWidget::decrease_brightness() {
    // Always get fresh brightness value before modifying
    get_current_brightness();
    
    int new_brightness = current_brightness - 5;
    if (new_brightness < 0) new_brightness = 0;
    
    set_brightness(new_brightness);
    show();
}

void BrightnessWidget::update_brightness(int brightness) {
    set_brightness(brightness);
}

GtkWidget* BrightnessWidget::get_widget() {
    return window;
}

// Static callbacks
gboolean BrightnessWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    BrightnessWidget *self = static_cast<BrightnessWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean BrightnessWidget::hide_timeout_static(gpointer user_data) {
    BrightnessWidget *self = static_cast<BrightnessWidget*>(user_data);
    if (self) {
        return self->hide_timeout();
    }
    return FALSE;
}

// Instance callbacks - Updated to match volume widget rounded design
gboolean BrightnessWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double width = allocation.width;
    double height = allocation.height;
    
    // Much more rounded radius - almost pill-shaped like your brightness widget
    double radius = width / 2.0; // This makes it very rounded on the sides
    
    // Ensure radius doesn't exceed half the height
    if (radius > height / 2.0) {
        radius = height / 2.0;
    }
    
    // Create very rounded rectangle (almost pill-shaped)
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    
    // Softer, more diffused drop shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.25);
    cairo_fill_preserve(cr);
    
    // Create the main background with subtle shadow offset
    cairo_translate(cr, -1, -1);
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);
    
    cairo_translate(cr, 1, 1);
    
    // Create gradient similar to your brightness widget
    cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, 0, height);
    if (g_use_hoc_theme) {
        // More subtle, refined colors for rounded design
        cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.75, 0.82, 0.95, 0.85);
        cairo_pattern_add_color_stop_rgba(gradient, 0.5, 0.68, 0.76, 0.90, 0.75);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.50, 0.58, 0.80, 0.65);
    } else {
        // Pink theme with softer gradient
        cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.95, 0.75, 0.85, 0.90);
        cairo_pattern_add_color_stop_rgba(gradient, 0.5, 0.98, 0.85, 0.92, 0.80);
        cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.94, 0.98, 0.70);
    }
    cairo_set_source(cr, gradient);
    cairo_fill_preserve(cr);

    // Subtle border for definition
    cairo_set_source_rgba(cr, 1, 1, 1, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    
    cairo_pattern_destroy(gradient);
    
    return FALSE;
}

gboolean BrightnessWidget::hide_timeout() {
    hide();
    return FALSE;
}

void BrightnessWidget::center_widget_vertically() {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkRectangle geom;
    int monitor = gdk_screen_get_primary_monitor(screen);
    gdk_screen_get_monitor_geometry(screen, monitor, &geom);
    
    int widget_height = 220; // Updated smaller widget height
    int screen_height = geom.height;
    int top_margin = (screen_height - widget_height) / 2;
    
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, top_margin);
}
