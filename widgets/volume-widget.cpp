#include "volume-widget.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <gtk-layer-shell.h>
#include <string>
#include <stdexcept>

VolumeWidget::VolumeWidget() : current_volume(50) {
    setup_window();
    setup_ui();
    apply_styles();
    
    get_current_volume();
    update_display();
}

VolumeWidget::~VolumeWidget() {
    if (hide_timer_id != 0) {
        g_source_remove(hide_timer_id);
    }
    if (window) {
        gtk_widget_destroy(window);
    }
}

void VolumeWidget::setup_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Volume Widget");
    // Smaller, more compact design
    gtk_window_set_default_size(GTK_WINDOW(window), 70, 220);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);

    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, FALSE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 20);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
    
    // Center the widget vertically
    center_widget_vertically();

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(window, visual);
    }
}

void VolumeWidget::setup_ui() {
    // Root container
    root_vbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root_vbox);
    
    // Card container with custom drawing
    card_container = gtk_event_box_new();
    gtk_widget_set_name(card_container, "volume-card");
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
    
    // Volume title
    volume_label = gtk_label_new("Volume");
    gtk_widget_set_name(volume_label, "volume-title");
    gtk_widget_set_halign(volume_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(title_value_box), volume_label, FALSE, FALSE, 0);
    
    // Volume value
    volume_value_label = gtk_label_new("50%");
    gtk_widget_set_name(volume_value_label, "volume-value");
    gtk_widget_set_halign(volume_value_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(title_value_box), volume_value_label, FALSE, FALSE, 0);
    
    // Progress section
    GtkWidget *progress_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(progress_section, 8);
    
    GtkWidget *progress_container = gtk_event_box_new();
    gtk_widget_set_size_request(progress_container, 16, 130);
    
    volume_progress = gtk_progress_bar_new();
    gtk_orientable_set_orientation(GTK_ORIENTABLE(volume_progress), GTK_ORIENTATION_VERTICAL);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(volume_progress), 0.0);
    gtk_widget_set_size_request(volume_progress, 6, 130);
    gtk_widget_set_name(volume_progress, "volume-progress");
    gtk_widget_set_halign(volume_progress, GTK_ALIGN_CENTER);
    
    gtk_container_add(GTK_CONTAINER(progress_container), volume_progress);
    gtk_box_pack_start(GTK_BOX(progress_section), progress_container, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), progress_section, FALSE, FALSE, 0);
    
    // Volume icon
    volume_icon = gtk_image_new_from_icon_name("light-volume", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_size_request(volume_icon, 24, 24);
    gtk_widget_set_halign(volume_icon, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(volume_icon, GTK_ALIGN_END);
    gtk_widget_set_name(volume_icon, "volume-icon");
    gtk_widget_set_margin_top(volume_icon, 8);
    gtk_box_pack_start(GTK_BOX(main_box), volume_icon, FALSE, FALSE, 0);
}

void VolumeWidget::apply_styles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    std::string css;
    
    if (g_use_hoc_theme) {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #volume-title { font-family: LightOSNew12; font-size: 13px; font-weight: 600; color: #ffffff; margin-bottom: 2px; text-align: center; }
            #volume-value { font-family: LightOSNew12; font-size: 12px; font-weight: 400; color: #ffffff; text-align: center; opacity: 0.9; }
            #volume-progress { background-color: rgba(255,255,255,0.3); border-radius: 3px; cursor: pointer; }
            #volume-progress progress { background: linear-gradient(180deg, #7077bd 0%, #b1c9ec 100%); border-radius: 3px; border: none; }
            #volume-icon { color: #ffffff; -gtk-icon-size: 22px; opacity: 0.95; }
        )";
    } else {
        css = R"(
            window { background-color: rgba(0,0,0,0); }
            #volume-title { font-family: LightOSNew12; font-size: 13px; font-weight: 600; color: #ffffff; margin-bottom: 2px; text-align: center; }
            #volume-value { font-family: LightOSNew12; font-size: 12px; font-weight: 400; color: #ffffff; text-align: center; opacity: 0.9; }
            #volume-progress { background-color: rgba(255,255,255,0.3); border-radius: 3px; cursor: pointer; }
            #volume-progress progress { background: linear-gradient(180deg, #ff6bb3 0%, #ffc4e1 100%); border-radius: 3px; border: none; }
            #volume-icon { color: #ffffff; -gtk-icon-size: 22px; opacity: 0.95; }
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

int VolumeWidget::safe_string_to_int(const std::string& str, int fallback) {
    try {
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

void VolumeWidget::get_current_volume() {
    int raw_volume = 50;
    
    FILE *pipe = popen("pamixer --get-volume 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            raw_volume = safe_string_to_int(std::string(buffer), 50);
            if (raw_volume < 0) raw_volume = 0;
            if (raw_volume > 100) raw_volume = 100;
        }
        pclose(pipe);
    }
    
    FILE *mute_pipe = popen("pamixer --get-mute 2>/dev/null", "r");
    if (mute_pipe) {
        char mute_buffer[128];
        if (fgets(mute_buffer, sizeof(mute_buffer), mute_pipe)) {
            std::string mute_str(mute_buffer);
            is_muted = (mute_str.find("true") != std::string::npos);
        }
        pclose(mute_pipe);
    }
    
    current_volume = raw_volume;
    
    std::cout << "Volume: raw=" << raw_volume << ", muted=" << (is_muted ? "true" : "false") << std::endl;
}

void VolumeWidget::set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    std::stringstream cmd;
    cmd << "pamixer --set-volume " << volume << " 2>/dev/null";
    
    int result = system(cmd.str().c_str());
    if (result == 0) {
        current_volume = volume;
        is_muted = false;
        std::cout << "Set volume to " << volume << "%" << std::endl;
    } else {
        std::cerr << "Failed to set volume to " << volume << "%" << std::endl;
        get_current_volume();
    }
    
    update_display();
}

void VolumeWidget::update_display() {
    std::string value_text = std::to_string(current_volume) + "%";
    if (is_muted) {
        value_text = "Muted";
    }
    gtk_label_set_text(GTK_LABEL(volume_value_label), value_text.c_str());
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(volume_progress), current_volume / 100.0);
}

void VolumeWidget::show() {
    if (!is_visible) {
        is_visible = true;
        get_current_volume();
        refresh_global_theme();
        apply_styles();
        gtk_widget_queue_draw(card_container);
        gtk_widget_show_all(window);
        
        if (hide_timer_id != 0) {
            g_source_remove(hide_timer_id);
        }
        hide_timer_id = g_timeout_add_seconds(2, hide_timeout_static, this);
    }
}

void VolumeWidget::hide() {
    if (is_visible) {
        is_visible = false;
        gtk_widget_hide(window);
        if (hide_timer_id != 0) {
            g_source_remove(hide_timer_id);
            hide_timer_id = 0;
        }
    }
}

bool VolumeWidget::get_visible() {
    return is_visible;
}

void VolumeWidget::increase_volume() {
    get_current_volume();
    
    int new_volume = current_volume + 5;
    if (new_volume > 100) new_volume = 100;
    
    set_volume(new_volume);
    show();
}

void VolumeWidget::decrease_volume() {
    get_current_volume();
    
    int new_volume = current_volume - 5;
    if (new_volume < 0) new_volume = 0;
    
    set_volume(new_volume);
    show();
}

void VolumeWidget::toggle_mute() {
    get_current_volume();
    
    std::stringstream cmd;
    cmd << "pamixer --toggle-mute 2>/dev/null";
    
    int result = system(cmd.str().c_str());
    if (result == 0) {
        get_current_volume();
        std::cout << "Toggled mute, now muted: " << (is_muted ? "true" : "false") << std::endl;
    } else {
        std::cerr << "Failed to toggle mute" << std::endl;
    }
    
    update_display();
    show();
}

void VolumeWidget::update_volume(int volume) {
    set_volume(volume);
}

GtkWidget* VolumeWidget::get_widget() {
    return window;
}

// Static callbacks
gboolean VolumeWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    VolumeWidget *self = static_cast<VolumeWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean VolumeWidget::hide_timeout_static(gpointer user_data) {
    VolumeWidget *self = static_cast<VolumeWidget*>(user_data);
    if (self) {
        return self->hide_timeout();
    }
    return FALSE;
}

// Instance callbacks - Rounded design
gboolean VolumeWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
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

gboolean VolumeWidget::hide_timeout() {
    hide();
    return FALSE;
}

void VolumeWidget::center_widget_vertically() {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
    GdkRectangle geom;
    int monitor = gdk_screen_get_primary_monitor(screen);
    gdk_screen_get_monitor_geometry(screen, monitor, &geom);
    
    int widget_height = 220; // Updated smaller widget height
    int screen_height = geom.height;
    int top_margin = (screen_height - widget_height) / 2;
    
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, top_margin);
}
