#include "host-widget.h"
#include "system-info-widget.h"
#include <unistd.h>
#include <pwd.h>

HostWidget::HostWidget() {
    // Get current username
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        current_username = pw->pw_name;
    } else {
        current_username = "user";
    }
    
    system_info_widget = nullptr;
    
    setup_ui();
    apply_styles();
}

GtkWidget* HostWidget::get_widget() {
    return host_container;
}

void HostWidget::update_username() {
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        current_username = pw->pw_name;
        gtk_label_set_text(GTK_LABEL(username_label), current_username.c_str());
    }
}

void HostWidget::setup_ui() {
    // Create main container with custom drawing like recent apps
    host_container = gtk_event_box_new();
    g_signal_connect(host_container, "draw", G_CALLBACK(on_draw_static), this);
    gtk_widget_set_size_request(host_container, -1, 60);
    
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(main_hbox, 16);
    gtk_widget_set_margin_end(main_hbox, 16);
    gtk_widget_set_margin_top(main_hbox, 8);
    gtk_widget_set_margin_bottom(main_hbox, 8);
    gtk_container_add(GTK_CONTAINER(host_container), main_hbox);
    
    // User icon - make it clickable using event box
    GtkWidget *user_icon_eventbox = gtk_event_box_new();
    user_icon = gtk_image_new();
    gtk_widget_set_size_request(user_icon, 28, 28);
    gtk_container_add(GTK_CONTAINER(user_icon_eventbox), user_icon);
    gtk_widget_set_tooltip_text(user_icon_eventbox, "Click to change user icon");
    g_signal_connect(user_icon_eventbox, "button-press-event", G_CALLBACK(on_user_icon_clicked_static), this);
    gtk_widget_set_margin_end(user_icon_eventbox, 8);
    
    // Load the user icon
    load_user_icon();
    
    // Username label
    username_label = gtk_label_new(current_username.c_str());
    gtk_widget_set_name(username_label, "host-username");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_widget_set_valign(username_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(username_label, 8);
    
    // Create actions section
    actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(actions_box, GTK_ALIGN_END);
    gtk_widget_set_valign(actions_box, GTK_ALIGN_CENTER);
    
    // Refresh button
    refresh_button = gtk_button_new();
    GtkWidget *refresh_icon = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_size_request(refresh_icon, 20, 20);
    gtk_container_add(GTK_CONTAINER(refresh_button), refresh_icon);
    gtk_widget_set_tooltip_text(refresh_button, "Refresh Art Image");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked_static), this);
    
    // Record button
    record_button = gtk_button_new();
    GtkWidget *record_icon = gtk_image_new_from_icon_name("media-record", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_size_request(record_icon, 20, 20);
    gtk_container_add(GTK_CONTAINER(record_button), record_icon);
    gtk_widget_set_tooltip_text(record_button, "Screen Record");
    g_signal_connect(record_button, "clicked", G_CALLBACK(on_record_clicked_static), this);
    
    // Screenshot button
    screenshot_button = gtk_button_new();
    GtkWidget *screenshot_icon = gtk_image_new_from_icon_name("camera-photo", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_size_request(screenshot_icon, 20, 20);
    gtk_container_add(GTK_CONTAINER(screenshot_button), screenshot_icon);
    gtk_widget_set_tooltip_text(screenshot_button, "Screenshot");
    g_signal_connect(screenshot_button, "clicked", G_CALLBACK(on_screenshot_clicked_static), this);
    
    // Power menu button
    powermenu_button = gtk_button_new();
    GtkWidget *powermenu_icon = gtk_image_new_from_icon_name("system-shutdown", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_size_request(powermenu_icon, 20, 20);
    gtk_container_add(GTK_CONTAINER(powermenu_button), powermenu_icon);
    gtk_widget_set_tooltip_text(powermenu_button, "Power Menu");
    g_signal_connect(powermenu_button, "clicked", G_CALLBACK(on_powermenu_clicked_static), this);
    
    gtk_box_pack_start(GTK_BOX(actions_box), refresh_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(actions_box), record_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(actions_box), screenshot_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(actions_box), powermenu_button, FALSE, FALSE, 0);
    
    // Pack everything in horizontal layout
    gtk_box_pack_start(GTK_BOX(main_hbox), user_icon_eventbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), username_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(main_hbox), actions_box, FALSE, FALSE, 0);
}

void HostWidget::apply_styles() {
    GtkCssProvider *css_provider = gtk_css_provider_new();
    
    const char *css = R"(
        #host-username {
            color: #ffffff;
            font-family: LightOSNew12;
            font-weight: 600;
        }
    )";
    
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

gboolean HostWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean HostWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    int width = allocation.width;
    int height = allocation.height;
    int radius = 16;
    
    // Simple rounded rectangle background
    cairo_new_sub_path(cr);
    cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
    cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
    cairo_close_path(cr);

    // Simple gradient background
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

void HostWidget::on_refresh_clicked_static(GtkButton *button, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    self->on_refresh_clicked();
}

void HostWidget::on_record_clicked_static(GtkButton *button, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    self->on_record_clicked();
}

void HostWidget::on_screenshot_clicked_static(GtkButton *button, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    self->on_screenshot_clicked();
}

void HostWidget::on_powermenu_clicked_static(GtkButton *button, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    self->on_powermenu_clicked();
}

void HostWidget::on_refresh_clicked() {
    if (system_info_widget) {
        system_info_widget->refresh_art_image();
    }
}

void HostWidget::on_record_clicked() {
    hide_stats_widget();
    system("~/.config/hypr/Scripts/gpu-screen-record &");
}

void HostWidget::on_screenshot_clicked() {
    hide_stats_widget();
    system("~/.config/hypr/Scripts/screenshot-region.sh &");
}

void HostWidget::on_powermenu_clicked() {
    hide_stats_widget();
    system("wlogout &");
}

void HostWidget::hide_stats_widget() {
    // Hide stats widget using the same approach as recent apps
    system("light-widget-daemon --stats >/dev/null 2>&1 &");
    system("./light-widget-daemon --stats >/dev/null 2>&1 &");
}

void HostWidget::load_user_icon() {
    std::string custom_icon_path = "/home/" + current_username + "/.config/Light/icon.png";
    
    // Check if custom icon exists
    if (access(custom_icon_path.c_str(), F_OK) == 0) {
        // Load custom icon
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(custom_icon_path.c_str(), NULL);
        if (pixbuf) {
            // Scale to 28x28
            GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 28, 28, GDK_INTERP_BILINEAR);
            gtk_image_set_from_pixbuf(GTK_IMAGE(user_icon), scaled_pixbuf);
            g_object_unref(scaled_pixbuf);
            g_object_unref(pixbuf);
            return;
        }
    }
    
    // Fallback to default icon
    gtk_image_set_from_icon_name(GTK_IMAGE(user_icon), "avatar-default", GTK_ICON_SIZE_LARGE_TOOLBAR);
}

void HostWidget::refresh_icon() {
    load_user_icon();
}

void HostWidget::set_system_info_widget(SystemInfoWidget *widget) {
    system_info_widget = widget;
}

gboolean HostWidget::on_user_icon_clicked_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    HostWidget *self = static_cast<HostWidget*>(user_data);
    self->on_user_icon_clicked();
    return TRUE;
}

void HostWidget::on_user_icon_clicked() {
    show_file_selector();
}

void HostWidget::show_file_selector() {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select User Icon",
                                                   NULL,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "Cancel", GTK_RESPONSE_CANCEL,
                                                   "Open", GTK_RESPONSE_ACCEPT,
                                                   NULL);
    
    // Set file filter for image files
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image Files");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    // Set current directory to home
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            copy_icon_file(std::string(filename));
            refresh_icon();
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void HostWidget::copy_icon_file(const std::string& source_path) {
    // Create directory if it doesn't exist
    std::string config_dir = "/home/" + current_username + "/.config/Light";
    std::string command = "mkdir -p " + config_dir;
    system(command.c_str());
    
    // Copy file to destination
    std::string dest_path = config_dir + "/icon.png";
    std::string copy_command = "cp \"" + source_path + "\" \"" + dest_path + "\"";
    system(copy_command.c_str());
}
