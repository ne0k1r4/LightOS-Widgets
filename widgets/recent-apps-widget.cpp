#include "recent-apps-widget.h"

// Static member definitions
std::unordered_map<std::string, std::string> RecentAppsWidget::icon_cache;
bool RecentAppsWidget::cache_initialized = false;

RecentAppsWidget::RecentAppsWidget() {
    setup_ui();
    initialize_icon_cache();
    load_recent_apps();
    update_display();
}

GtkWidget* RecentAppsWidget::get_widget() {
    return apps_container;
}

void RecentAppsWidget::refresh() {
    load_recent_apps();
    update_display();
}

void RecentAppsWidget::initialize_icon_cache() {
    if (cache_initialized) return;
    
    std::vector<std::string> desktop_dirs = {
        "/usr/share/applications/",
        "/usr/local/share/applications/",
        "/home/" + std::string(g_get_user_name()) + "/.local/share/applications/"
    };
    
    for (const auto& desktop_dir : desktop_dirs) {
        std::string find_cmd = "find " + desktop_dir + " -name '*.desktop' 2>/dev/null";
        FILE* pipe = popen(find_cmd.c_str(), "r");
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string desktop_path = buffer;
                desktop_path.erase(desktop_path.find_last_not_of("\n\r") + 1);
                
                std::ifstream file(desktop_path);
                if (file.is_open()) {
                    std::string line;
                    std::string name_line, icon_line;
                    bool found_name = false, found_icon = false;
                    
                    while (std::getline(file, line)) {
                        if (line.find("Name=") == 0) {
                            name_line = line.substr(5);
                            name_line.erase(0, name_line.find_first_not_of(" \t"));
                            name_line.erase(name_line.find_last_not_of(" \t") + 1);
                            found_name = true;
                        } else if (line.find("Icon=") == 0) {
                            icon_line = line.substr(5);
                            icon_line.erase(0, icon_line.find_first_not_of(" \t"));
                            icon_line.erase(icon_line.find_last_not_of(" \t") + 1);
                            found_icon = true;
                        }
                        
                        if (found_name && found_icon) {
                            if (!name_line.empty() && !icon_line.empty()) {
                                icon_cache[name_line] = icon_line;
                            }
                            break;
                        }
                    }
                    file.close();
                }
            }
            pclose(pipe);
        }
    }
    
    std::cout << "DEBUG: Icon cache initialized with " << icon_cache.size() << " entries." << std::endl;
    
    // Debug: Print some cached entries
    int count = 0;
    for (const auto& pair : icon_cache) {
        if (count < 10) {
            std::cout << "DEBUG: Cached: " << pair.first << " -> " << pair.second << std::endl;
            count++;
        }
    }
    
    cache_initialized = true;
}

void RecentAppsWidget::setup_ui() {
    apps_container = gtk_event_box_new();
    g_signal_connect(apps_container, "draw", G_CALLBACK(on_draw_static), this);
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(main_vbox, 16); // Original margin
    gtk_widget_set_margin_end(main_vbox, 16); // Original margin
    gtk_widget_set_margin_top(main_vbox, 12); // Original margin
    gtk_widget_set_margin_bottom(main_vbox, 16); // Original margin
    gtk_container_add(GTK_CONTAINER(apps_container), main_vbox);
    
    apps_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(apps_grid), 8); // Original spacing
    gtk_grid_set_column_spacing(GTK_GRID(apps_grid), 8); // Original spacing
    gtk_widget_set_halign(apps_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_vbox), apps_grid, TRUE, TRUE, 0);
}

void RecentAppsWidget::load_recent_apps() {
    recent_apps.clear();
    
    const char* home = g_get_home_dir();
    std::string cache_path = std::string(home ? home : "") + "/.cache/ely_launcher_cache.json";
    
    std::ifstream file(cache_path);
    if (!file.is_open()) {
        return;
    }
    
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(file, root)) {
        file.close();
        return;
    }
    file.close();
    
    if (!root.isMember("data") || !root["data"].isObject()) {
        return;
    }
    
    Json::Value data = root["data"];
    std::vector<std::pair<std::string, int>> app_usage;
    
    // Parse all apps and their usage counts
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it->isInt()) {
            std::string app_name = it.key().asString();
            int usage_count = it->asInt();
            app_usage.push_back({app_name, usage_count});
        }
    }
    
    // Sort by usage count (descending) - highest hit numbers first
    std::sort(app_usage.begin(), app_usage.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Take top 6 apps with highest usage counts
    for (int i = 0; i < std::min(6, (int)app_usage.size()); i++) {
        AppInfo app;
        app.name = app_usage[i].first;
        app.usage_count = app_usage[i].second;
        app.icon_path = find_app_icon(app.name);
        
        // Debug output
        std::cout << "DEBUG: App " << (i+1) << ": " << app.name << " -> Icon: " << app.icon_path << std::endl;
        
        recent_apps.push_back(app);
    }
}

std::string RecentAppsWidget::find_app_icon(const std::string& app_name) {
    // First, try to get icon from desktop file
    std::string desktop_icon = get_icon_from_desktop_file(app_name);
    if (!desktop_icon.empty()) {
        return desktop_icon;
    }
    
    // Try common icon locations with various extensions
    std::vector<std::string> icon_paths = {
        "/usr/share/pixmaps/" + app_name + ".png",
        "/usr/share/pixmaps/" + app_name + ".svg",
        "/usr/share/pixmaps/" + app_name + ".xpm",
        "/usr/share/icons/hicolor/48x48/apps/" + app_name + ".png",
        "/usr/share/icons/hicolor/48x48/apps/" + app_name + ".svg",
        "/usr/share/icons/hicolor/48x48/apps/" + app_name + ".xpm",
        "/usr/share/icons/hicolor/32x32/apps/" + app_name + ".png",
        "/usr/share/icons/hicolor/32x32/apps/" + app_name + ".svg",
        "/usr/share/icons/hicolor/32x32/apps/" + app_name + ".xpm"
    };
    
    for (const auto& path : icon_paths) {
        if (access(path.c_str(), F_OK) == 0) {
            return path;
        }
    }
    
    // Try to find icon using gtk-icon-theme
    std::string gtk_icon = find_icon_with_gtk_theme(app_name);
    if (!gtk_icon.empty()) {
        return gtk_icon;
    }
    
    // Return a default icon path
    return "/usr/share/pixmaps/application-x-executable.png";
}

std::string RecentAppsWidget::get_icon_from_desktop_file(const std::string& app_name) {
    // First try exact match in cache
    auto it = icon_cache.find(app_name);
    if (it != icon_cache.end()) {
        std::string theme_icon = find_icon_with_gtk_theme(it->second);
        if (!theme_icon.empty()) {
            std::cout << "DEBUG: Found exact match for " << app_name << " -> " << it->second << std::endl;
            return theme_icon;
        }
    }
    
    // Try case-insensitive exact match
    std::string lower_app_name = app_name;
    std::transform(lower_app_name.begin(), lower_app_name.end(), lower_app_name.begin(), ::tolower);
    
    for (const auto& pair : icon_cache) {
        const std::string& cached_name = pair.first;
        const std::string& cached_icon = pair.second;
        
        std::string lower_cached_name = cached_name;
        std::transform(lower_cached_name.begin(), lower_cached_name.end(), lower_cached_name.begin(), ::tolower);
        
        if (lower_cached_name == lower_app_name) {
            std::string theme_icon = find_icon_with_gtk_theme(cached_icon);
            if (!theme_icon.empty()) {
                std::cout << "DEBUG: Found case-insensitive match for " << app_name << " -> " << cached_icon << std::endl;
                return theme_icon;
            }
        }
    }
    
    // Try partial matches in cache (more strict matching)
    for (const auto& pair : icon_cache) {
        const std::string& cached_name = pair.first;
        const std::string& cached_icon = pair.second;
        
        // Check if app_name is contained in cached_name or vice versa
        if ((cached_name.find(app_name) != std::string::npos && cached_name.length() - app_name.length() <= 5) ||
            (app_name.find(cached_name) != std::string::npos && app_name.length() - cached_name.length() <= 5)) {
            
            std::string theme_icon = find_icon_with_gtk_theme(cached_icon);
            if (!theme_icon.empty()) {
                std::cout << "DEBUG: Found partial match for " << app_name << " -> " << cached_icon << " (from " << cached_name << ")" << std::endl;
                return theme_icon;
            }
        }
    }
    
    std::cout << "DEBUG: No match found for " << app_name << std::endl;
    return "";
}

std::string RecentAppsWidget::find_icon_with_gtk_theme(const std::string& icon_name) {
    // Use gtk-icon-theme to find the icon
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
    if (!icon_theme) {
        std::cout << "DEBUG: No GTK icon theme available" << std::endl;
        return "";
    }
    
    std::cout << "DEBUG: Looking for icon: " << icon_name << std::endl;
    
    // Try different sizes
    std::vector<int> sizes = {48, 32, 24, 16, 64, 128};
    for (int size : sizes) {
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(icon_theme, icon_name.c_str(), size, GTK_ICON_LOOKUP_FORCE_SIZE);
        if (icon_info) {
            std::string filename = gtk_icon_info_get_filename(icon_info);
            g_object_unref(icon_info);
            if (!filename.empty() && access(filename.c_str(), F_OK) == 0) {
                std::cout << "DEBUG: Found icon at: " << filename << std::endl;
                return filename;
            }
        }
    }
    
    // Try without size constraint
    GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(icon_theme, icon_name.c_str(), 48, GTK_ICON_LOOKUP_NO_SVG);
    if (icon_info) {
        std::string filename = gtk_icon_info_get_filename(icon_info);
        g_object_unref(icon_info);
        if (!filename.empty() && access(filename.c_str(), F_OK) == 0) {
            std::cout << "DEBUG: Found icon at: " << filename << std::endl;
            return filename;
        }
    }
    
    std::cout << "DEBUG: Icon not found in theme: " << icon_name << std::endl;
    return "";
}

void RecentAppsWidget::update_display() {
    // Clear existing grid children and properly clean up memory
    GList *children = gtk_container_get_children(GTK_CONTAINER(apps_grid));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        GtkWidget *widget = GTK_WIDGET(iter->data);
        // Get the user data (app name) and free it before destroying the widget
        gpointer user_data = g_object_get_data(G_OBJECT(widget), "app-name");
        if (user_data) {
            g_free(user_data);
        }
        
        // No custom hover state cleanup needed
        
        gtk_widget_destroy(widget);
    }
    g_list_free(children);
    
    // Add app buttons
    for (size_t i = 0; i < recent_apps.size(); i++) {
        const auto& app = recent_apps[i];
        
        GtkWidget *app_button = gtk_button_new();
        gtk_widget_set_name(app_button, "app-button");
        gtk_widget_set_size_request(app_button, 48, 48); // Original size
        gtk_button_set_relief(GTK_BUTTON(app_button), GTK_RELIEF_NONE);
        
        // No custom hover state needed - using CSS hover effects
        
        // Create icon and label container
        GtkWidget *app_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        
        // Try to load icon
        GtkWidget *icon_widget = nullptr;
        if (!app.icon_path.empty() && access(app.icon_path.c_str(), F_OK) == 0) {
            GError *error = nullptr;
            GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
                app.icon_path.c_str(), 24, 24, TRUE, &error);
            if (pixbuf) {
                icon_widget = gtk_image_new_from_pixbuf(pixbuf);
                g_object_unref(pixbuf);
            } else if (error) {
                g_error_free(error);
            }
        }
        
        // If still no icon, try to get it from GTK icon theme directly
        if (!icon_widget) {
            GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
            if (icon_theme) {
                // Try the app name directly first
                GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(icon_theme, app.name.c_str(), 24, GTK_ICON_LOOKUP_FORCE_SIZE);
                if (icon_info) {
                    GdkPixbuf *pixbuf = gtk_icon_info_load_icon(icon_info, nullptr);
                    if (pixbuf) {
                        icon_widget = gtk_image_new_from_pixbuf(pixbuf);
                        g_object_unref(pixbuf);
                    }
                    g_object_unref(icon_info);
                }
            }
        }
        
        if (!icon_widget) {
            // Fallback to text icon
            icon_widget = gtk_label_new("📱");
            gtk_widget_set_name(icon_widget, "app-icon-fallback");
        } else {
            gtk_widget_set_name(icon_widget, "app-icon");
        }
        
        gtk_box_pack_start(GTK_BOX(app_box), icon_widget, FALSE, FALSE, 0);
        
        // App name (truncated)
        std::string display_name = app.name;
        if (display_name.length() > 8) {
            display_name = display_name.substr(0, 8) + "...";
        }
        
        GtkWidget *name_label = gtk_label_new(display_name.c_str());
        gtk_widget_set_name(name_label, "app-name");
        gtk_label_set_ellipsize(GTK_LABEL(name_label), PANGO_ELLIPSIZE_END);
        gtk_box_pack_start(GTK_BOX(app_box), name_label, FALSE, FALSE, 0);
        
        gtk_container_add(GTK_CONTAINER(app_button), app_box);
        
        // Store app name as widget data and connect click handler
        g_object_set_data(G_OBJECT(app_button), "app-name", g_strdup(app.name.c_str()));
        g_signal_connect(app_button, "clicked", G_CALLBACK(on_app_clicked_static), NULL);
        
        // Hover effects are handled by CSS - no need for custom mouse events
        
        gtk_grid_attach(GTK_GRID(apps_grid), app_button, i % 3, i / 3, 1, 1);
    }
    
    gtk_widget_show_all(apps_container);
}

gboolean RecentAppsWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    RecentAppsWidget *self = static_cast<RecentAppsWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean RecentAppsWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
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

std::string RecentAppsWidget::find_desktop_file_name(const std::string& app_name) {
    std::vector<std::string> desktop_dirs = {
        "/usr/share/applications/",
        "/usr/local/share/applications/",
        "/home/" + std::string(g_get_user_name()) + "/.local/share/applications/"
    };
    
    for (const auto& desktop_dir : desktop_dirs) {
        std::string find_cmd = "find " + desktop_dir + " -name '*.desktop' 2>/dev/null";
        FILE* pipe = popen(find_cmd.c_str(), "r");
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string desktop_path = buffer;
                desktop_path.erase(desktop_path.find_last_not_of("\n\r") + 1);
                
                // Read the desktop file to check the Name field
                std::ifstream file(desktop_path);
                if (file.is_open()) {
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find("Name=") == 0) {
                            std::string desktop_name = line.substr(5); // Remove "Name="
                            if (desktop_name == app_name) {
                                file.close();
                                pclose(pipe);
                                // Extract just the filename without path and extension
                                size_t last_slash = desktop_path.find_last_of("/");
                                size_t last_dot = desktop_path.find_last_of(".");
                                if (last_slash != std::string::npos && last_dot != std::string::npos) {
                                    return desktop_path.substr(last_slash + 1, last_dot - last_slash - 1);
                                }
                            }
                        }
                    }
                    file.close();
                }
            }
            pclose(pipe);
        }
    }
    
    return ""; // Not found
}

void RecentAppsWidget::on_app_clicked_static(GtkButton *button, gpointer user_data) {
    // Get app name from widget data
    char *app_name = static_cast<char*>(g_object_get_data(G_OBJECT(button), "app-name"));
    if (!app_name) {
        return; // No app name found
    }
    
    // Hide all widgets when launching an app using the command approach
    // Try system PATH first, then fallback to current directory
    system("light-widget-daemon --stats >/dev/null 2>&1 &");
    system("./light-widget-daemon --stats >/dev/null 2>&1 &");
    
    // Find the correct desktop file name
    std::string desktop_file_name = find_desktop_file_name(std::string(app_name));
    
    if (!desktop_file_name.empty()) {
        // Launch using the correct desktop file name
        std::string command = "gtk-launch " + desktop_file_name + " &";
        system(command.c_str());
    } else {
        // Fallback: try to launch directly with the app name (for apps like kitty that match)
        std::string command = "gtk-launch " + std::string(app_name) + " &";
        system(command.c_str());
    }
}


