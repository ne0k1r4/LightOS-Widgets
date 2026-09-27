#include "system-info-widget.h"
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <sstream>

// Callback function for libcurl to write data to string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    }
    catch(std::bad_alloc &e) {
        return 0;
    }
}

SystemInfoWidget::SystemInfoWidget() {
    // Initialize random seed for art rotation
    srand(time(nullptr));
    setup_ui();
    load_art_files();
    start_update_timer();
    // Load existing art image if available
    load_existing_art();
}

SystemInfoWidget::~SystemInfoWidget() {
    if (update_timer_id > 0) {
        g_source_remove(update_timer_id);
    }
}

GtkWidget* SystemInfoWidget::get_widget() {
    return system_container;
}

void SystemInfoWidget::refresh() {
    load_art_files();
    load_existing_art();
}

void SystemInfoWidget::setup_ui() {
    system_container = gtk_event_box_new();
    g_signal_connect(system_container, "draw", G_CALLBACK(on_draw_static), this);
    
    // Main vertical container - stats on top, art below
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(main_vbox, 16);
    gtk_widget_set_margin_end(main_vbox, 16);
    gtk_widget_set_margin_top(main_vbox, 12);
    gtk_widget_set_margin_bottom(main_vbox, 16);
    gtk_container_add(GTK_CONTAINER(system_container), main_vbox);
    
    // Top section - System stats
    stats_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(stats_vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(stats_vbox, GTK_ALIGN_CENTER);
    
    // Stats grid - 2x2 layout for system metrics
    stats_box = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(stats_box), 15);
    gtk_grid_set_column_spacing(GTK_GRID(stats_box), 20);
    gtk_widget_set_halign(stats_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(stats_vbox), stats_box, TRUE, TRUE, 0);
    
    // CPU widget - back to original size since we have more space
    GtkWidget *cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(cpu_box, GTK_ALIGN_CENTER);
    
    cpu_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(cpu_canvas, 80, 80);
    gtk_widget_set_events(cpu_canvas, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_object_set_data(G_OBJECT(cpu_canvas), "system-info-widget", this);
    g_signal_connect(cpu_canvas, "draw", G_CALLBACK(on_cpu_draw_static), this);
    g_signal_connect(cpu_canvas, "enter-notify-event", G_CALLBACK(on_mouse_enter_static), (gpointer)"cpu");
    g_signal_connect(cpu_canvas, "leave-notify-event", G_CALLBACK(on_mouse_leave_static), (gpointer)"cpu");
    gtk_box_pack_start(GTK_BOX(cpu_box), cpu_canvas, FALSE, FALSE, 0);
    
    cpu_label = gtk_label_new("CPU");
    gtk_widget_set_name(cpu_label, "system-label");
    gtk_widget_set_halign(cpu_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(cpu_box), cpu_label, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(stats_box), cpu_box, 0, 0, 1, 1);
    
    // RAM widget
    GtkWidget *ram_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(ram_box, GTK_ALIGN_CENTER);
    
    ram_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(ram_canvas, 80, 80);
    gtk_widget_set_events(ram_canvas, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_object_set_data(G_OBJECT(ram_canvas), "system-info-widget", this);
    g_signal_connect(ram_canvas, "draw", G_CALLBACK(on_ram_draw_static), this);
    g_signal_connect(ram_canvas, "enter-notify-event", G_CALLBACK(on_mouse_enter_static), (gpointer)"ram");
    g_signal_connect(ram_canvas, "leave-notify-event", G_CALLBACK(on_mouse_leave_static), (gpointer)"ram");
    gtk_box_pack_start(GTK_BOX(ram_box), ram_canvas, FALSE, FALSE, 0);
    
    ram_label = gtk_label_new("RAM");
    gtk_widget_set_name(ram_label, "system-label");
    gtk_widget_set_halign(ram_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(ram_box), ram_label, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(stats_box), ram_box, 1, 0, 1, 1);
    
    // TEMP widget
    GtkWidget *temp_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(temp_box, GTK_ALIGN_CENTER);
    
    temp_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(temp_canvas, 80, 80);
    gtk_widget_set_events(temp_canvas, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_object_set_data(G_OBJECT(temp_canvas), "system-info-widget", this);
    g_signal_connect(temp_canvas, "draw", G_CALLBACK(on_temp_draw_static), this);
    g_signal_connect(temp_canvas, "enter-notify-event", G_CALLBACK(on_mouse_enter_static), (gpointer)"temp");
    g_signal_connect(temp_canvas, "leave-notify-event", G_CALLBACK(on_mouse_leave_static), (gpointer)"temp");
    gtk_box_pack_start(GTK_BOX(temp_box), temp_canvas, FALSE, FALSE, 0);
    
    temp_label = gtk_label_new("TEMP");
    gtk_widget_set_name(temp_label, "system-label");
    gtk_widget_set_halign(temp_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(temp_box), temp_label, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(stats_box), temp_box, 0, 1, 1, 1);
    
    // DISK widget
    GtkWidget *disk_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(disk_box, GTK_ALIGN_CENTER);
    
    disk_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(disk_canvas, 80, 80);
    gtk_widget_set_events(disk_canvas, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_object_set_data(G_OBJECT(disk_canvas), "system-info-widget", this);
    g_signal_connect(disk_canvas, "draw", G_CALLBACK(on_disk_draw_static), this);
    g_signal_connect(disk_canvas, "enter-notify-event", G_CALLBACK(on_mouse_enter_static), (gpointer)"disk");
    g_signal_connect(disk_canvas, "leave-notify-event", G_CALLBACK(on_mouse_leave_static), (gpointer)"disk");
    gtk_box_pack_start(GTK_BOX(disk_box), disk_canvas, FALSE, FALSE, 0);
    
    disk_label = gtk_label_new("DISK");
    gtk_widget_set_name(disk_label, "system-label");
    gtk_widget_set_halign(disk_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(disk_box), disk_label, FALSE, FALSE, 0);
    
    gtk_grid_attach(GTK_GRID(stats_box), disk_box, 1, 1, 1, 1);
    
    // Pack stats section to main vertical container
    gtk_box_pack_start(GTK_BOX(main_vbox), stats_vbox, FALSE, FALSE, 0);
    
    // Bottom section - Art display
    art_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(art_vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(art_vbox, GTK_ALIGN_CENTER);
    
    // Create art image widget - fixed size 1423x2048
    art_image = gtk_image_new();
    gtk_widget_set_size_request(art_image, 300, 400);
    gtk_widget_set_halign(art_image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(art_image, GTK_ALIGN_CENTER);
    
    // No frame - just add the image directly for transparent background
    gtk_box_pack_start(GTK_BOX(art_vbox), art_image, TRUE, TRUE, 0);
    
    // Pack art section to main vertical container
    gtk_box_pack_start(GTK_BOX(main_vbox), art_vbox, TRUE, TRUE, 0);
}

void SystemInfoWidget::update_system_stats() {
    // Get CPU usage
    static unsigned long long prev_idle = 0, prev_total = 0;
    std::ifstream file("/proc/stat");
    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string cpu;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
        unsigned long long total_idle = idle + iowait;
        
        if (prev_total > 0) {
            unsigned long long total_diff = total - prev_total;
            unsigned long long idle_diff = total_idle - prev_idle;
            cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
            cpu_usage = std::max(0.0, std::min(100.0, cpu_usage));
        }
        
        prev_total = total;
        prev_idle = total_idle;
    }
    file.close();
    
    // Get RAM usage
    std::ifstream memfile("/proc/meminfo");
    unsigned long long total_mem = 0, free_mem = 0, available_mem = 0;
    std::string memline;
    
    while (std::getline(memfile, memline)) {
        if (memline.find("MemTotal:") == 0) {
            std::istringstream iss(memline);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            total_mem = std::stoull(value) * 1024; // Convert from KB to bytes
        } else if (memline.find("MemAvailable:") == 0) {
            std::istringstream iss(memline);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            available_mem = std::stoull(value) * 1024; // Convert from KB to bytes
        }
    }
    memfile.close();
    
    if (total_mem > 0) {
        ram_usage = 100.0 * (total_mem - available_mem) / total_mem;
        ram_usage = std::max(0.0, std::min(100.0, ram_usage));
    }
    
    // Get CPU temperature (try multiple methods)
    temp_usage = 0.0;
    
    // Method 1: Try sensors command with robust parsing
    FILE* sensors_pipe = popen("LC_ALL=C sensors 2>/dev/null", "r");
    if (sensors_pipe) {
        char buffer[512];
        std::vector<double> cpu_temps;
        
        while (fgets(buffer, sizeof(buffer), sensors_pipe)) {
            std::string line(buffer);
            
            // Look for CPU-related temperature lines (more comprehensive patterns)
            bool is_cpu_temp = (line.find("CPU:") != std::string::npos || 
                               line.find("Tctl:") != std::string::npos ||
                               line.find("Core 0:") != std::string::npos ||
                               line.find("Package id 0:") != std::string::npos ||
                               line.find("edge:") != std::string::npos ||
                               line.find("Composite:") != std::string::npos ||
                               line.find("Tdie:") != std::string::npos ||
                               line.find("Tccd1:") != std::string::npos);
            
            if (is_cpu_temp) {
                // Extract temperature value using more robust approach
                // Look for patterns like: +58.0°C, -270.1°C, 59.8°C
                size_t temp_start = std::string::npos;
                
                // Try different temperature patterns
                std::vector<std::string> patterns = {"+", "-", " "};
                for (const auto& pattern : patterns) {
                    temp_start = line.find(pattern);
                    if (temp_start != std::string::npos) {
                        // Make sure it's followed by a digit
                        if (temp_start + 1 < line.length() && std::isdigit(line[temp_start + 1])) {
                            break;
                        }
                    }
                }
                
                if (temp_start != std::string::npos) {
                    // Find the end of the temperature value
                    size_t temp_end = line.find("°C", temp_start);
                    if (temp_end == std::string::npos) {
                        // Try without degree symbol
                        temp_end = line.find("C", temp_start);
                    }
                    if (temp_end == std::string::npos) {
                        // Try to find end by looking for space or other delimiters
                        temp_end = temp_start + 1;
                        while (temp_end < line.length() && 
                               (std::isdigit(line[temp_end]) || line[temp_end] == '.' || line[temp_end] == '-')) {
                            temp_end++;
                        }
                    }
                    
                    if (temp_end != std::string::npos && temp_end > temp_start) {
                        std::string temp_str = line.substr(temp_start, temp_end - temp_start);
                        try {
                            double temp_celsius = std::stod(temp_str);
                            // Only accept reasonable temperature values (exclude invalid readings like -270°C)
                            if (temp_celsius > -50.0 && temp_celsius < 150.0) {
                                cpu_temps.push_back(temp_celsius);
                            }
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
            }
        }
        pclose(sensors_pipe);
        
        // Use the highest CPU temperature found (most likely to be accurate)
        if (!cpu_temps.empty()) {
            double max_temp = *std::max_element(cpu_temps.begin(), cpu_temps.end());
            actual_temp = max_temp;  // Store actual temperature
            // Convert to percentage for visual progress bar (0-100°C = 0-100%)
            // This gives a more intuitive visual representation
            temp_usage = std::max(0.0, std::min(100.0, max_temp));
        }
    }
    
    // Method 2: If sensors failed, try hwmon files with dynamic discovery
    if (temp_usage == 0.0) {
        std::vector<double> hwmon_temps;
        
        // Try to find all available hwmon temperature files
        FILE* find_pipe = popen("LC_ALL=C find /sys/class/hwmon -name 'temp*_input' 2>/dev/null", "r");
        if (find_pipe) {
            char path_buffer[256];
            while (fgets(path_buffer, sizeof(path_buffer), find_pipe)) {
                std::string path(path_buffer);
                path.erase(path.find_last_not_of("\n\r") + 1); // Remove newline
                
                std::ifstream tempfile(path);
                if (tempfile.is_open()) {
                    std::string temp_str;
                    if (std::getline(tempfile, temp_str)) {
                        try {
                            double temp_millicelsius = std::stod(temp_str);
                            // Only accept reasonable temperature values (0-150°C in millidegrees)
                            if (temp_millicelsius > 0 && temp_millicelsius < 150000) {
                                double temp_celsius = temp_millicelsius / 1000.0;
                                hwmon_temps.push_back(temp_celsius);
                            }
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                    tempfile.close();
                }
            }
            pclose(find_pipe);
        }
        
        // Use the highest temperature found from hwmon (most likely CPU)
        if (!hwmon_temps.empty()) {
            double max_temp = *std::max_element(hwmon_temps.begin(), hwmon_temps.end());
            actual_temp = max_temp;  // Store actual temperature
            // Convert to percentage for visual progress bar (0-100°C = 0-100%)
            temp_usage = std::max(0.0, std::min(100.0, max_temp));
        }
    }
    
    // Method 3: Fallback to thermal zones with dynamic discovery
    if (temp_usage == 0.0) {
        std::vector<double> thermal_temps;
        
        // Try to find all available thermal zone files
        FILE* find_thermal_pipe = popen("LC_ALL=C find /sys/class/thermal -name 'temp' 2>/dev/null", "r");
        if (find_thermal_pipe) {
            char path_buffer[256];
            while (fgets(path_buffer, sizeof(path_buffer), find_thermal_pipe)) {
                std::string path(path_buffer);
                path.erase(path.find_last_not_of("\n\r") + 1); // Remove newline
                
                std::ifstream tempfile(path);
                if (tempfile.is_open()) {
                    std::string temp_str;
                    if (std::getline(tempfile, temp_str)) {
                        try {
                            double temp_millicelsius = std::stod(temp_str);
                            // Only accept reasonable temperature values (0-150°C in millidegrees)
                            if (temp_millicelsius > 0 && temp_millicelsius < 150000) {
                                double temp_celsius = temp_millicelsius / 1000.0;
                                thermal_temps.push_back(temp_celsius);
                            }
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                    tempfile.close();
                }
            }
            pclose(find_thermal_pipe);
        }
        
        // Use the highest temperature found from thermal zones
        if (!thermal_temps.empty()) {
            double max_temp = *std::max_element(thermal_temps.begin(), thermal_temps.end());
            actual_temp = max_temp;  // Store actual temperature
            // Convert to percentage for visual progress bar (0-100°C = 0-100%)
            temp_usage = std::max(0.0, std::min(100.0, max_temp));
        }
    }
    
    // Method 4: Final fallback - try ACPI thermal zones (older systems)
    if (temp_usage == 0.0) {
        std::vector<std::string> acpi_paths = {
            "/proc/acpi/thermal_zone/THM0/temperature",
            "/proc/acpi/thermal_zone/THM1/temperature",
            "/proc/acpi/thermal_zone/THRM/temperature"
        };
        
        for (const auto& path : acpi_paths) {
            std::ifstream tempfile(path);
            if (tempfile.is_open()) {
                std::string temp_str;
                if (std::getline(tempfile, temp_str)) {
                    try {
                        // ACPI format is usually "temperature: 58 C"
                        size_t colon_pos = temp_str.find(":");
                        if (colon_pos != std::string::npos) {
                            std::string temp_part = temp_str.substr(colon_pos + 1);
                            size_t c_pos = temp_part.find("C");
                            if (c_pos != std::string::npos) {
                                std::string temp_value = temp_part.substr(0, c_pos);
                                // Remove whitespace
                                temp_value.erase(0, temp_value.find_first_not_of(" \t"));
                                temp_value.erase(temp_value.find_last_not_of(" \t") + 1);
                                
                                double temp_celsius = std::stod(temp_value);
                                if (temp_celsius > 0 && temp_celsius < 150) {
                                    actual_temp = temp_celsius;  // Store actual temperature
                                    // Convert to percentage for visual progress bar (0-100°C = 0-100%)
                                    temp_usage = std::max(0.0, std::min(100.0, temp_celsius));
                                    break; // Found valid temperature
                                }
                            }
                        }
                    } catch (...) {
                        // Ignore parsing errors
                    }
                }
                tempfile.close();
            }
        }
    }
    
    // Get overall disk usage (all mounted filesystems)
    disk_usage = 0.0;
    FILE* df_pipe = popen("LC_ALL=C df -h --total 2>/dev/null | tail -1", "r");
    if (df_pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), df_pipe)) {
            std::string line(buffer);
            // Parse the total line: "total 100G 50G 50G 50% /"
            std::istringstream iss(line);
            std::string total, size, used, avail, percent_str, mount;
            if (iss >> total >> size >> used >> avail >> percent_str) {
                try {
                    // Remove % sign and convert to double
                    percent_str.erase(std::remove(percent_str.begin(), percent_str.end(), '%'), percent_str.end());
                    disk_usage = std::stod(percent_str);
                    disk_usage = std::max(0.0, std::min(100.0, disk_usage));
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        }
        pclose(df_pipe);
    }
    
    // Update labels
    char cpu_text[32], ram_text[32], temp_text[32], disk_text[32];
    snprintf(cpu_text, sizeof(cpu_text), "CPU\n%.0f%%", cpu_usage);
    snprintf(ram_text, sizeof(ram_text), "RAM\n%.0f%%", ram_usage);
    snprintf(temp_text, sizeof(temp_text), "TEMP\n%.0f°C", actual_temp);
    snprintf(disk_text, sizeof(disk_text), "DISK\n%.0f%%", disk_usage);
    
    gtk_label_set_text(GTK_LABEL(cpu_label), cpu_text);
    gtk_label_set_text(GTK_LABEL(ram_label), ram_text);
    gtk_label_set_text(GTK_LABEL(temp_label), temp_text);
    gtk_label_set_text(GTK_LABEL(disk_label), disk_text);
    
    // Redraw canvases
    gtk_widget_queue_draw(cpu_canvas);
    gtk_widget_queue_draw(ram_canvas);
    gtk_widget_queue_draw(temp_canvas);
    gtk_widget_queue_draw(disk_canvas);
}

void SystemInfoWidget::start_update_timer() {
    update_timer_id = g_timeout_add(1000, [](gpointer user_data) -> gboolean {
        SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
        self->update_system_stats();
        return TRUE;
    }, this);
}

// Art widget methods
void SystemInfoWidget::load_art_files() {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

// Safebooru API methods
std::string SystemInfoWidget::fetch_random_image_url() {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    
    curl = curl_easy_init();
    if(curl) {
        // Safebooru API endpoint for random post with specific tag
        std::string api_url = "https://safebooru.donmai.us/posts.json?tags=death_note&limit=1&random=true";
        
        curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LightOSWidget/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return "";
        }
        
        curl_easy_cleanup(curl);
    }
    
    // Parse JSON response
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(readBuffer, root)) {
        if (root.isArray() && root.size() > 0) {
            Json::Value post = root[0];
            if (post.isMember("file_url")) {
                return post["file_url"].asString();
            }
        }
    }
    
    return "";
}

bool SystemInfoWidget::download_image(const std::string& url, const std::string& filepath) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    
    curl = curl_easy_init();
    if(curl) {
        fp = fopen(filepath.c_str(), "wb");
        if(fp) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "LightOSWidget/1.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            
            res = curl_easy_perform(curl);
            fclose(fp);
            
            if(res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return false;
            }
            
            curl_easy_cleanup(curl);
            return true;
        }
        curl_easy_cleanup(curl);
    }
    
    return false;
}

// Struct for passing data to UI update callback
struct ArtUpdateData {
    SystemInfoWidget *widget;
    std::string temp_file;
};

// Static callback for updating UI from main thread
static gboolean update_art_image_callback(gpointer data) {
    ArtUpdateData *update_data = static_cast<ArtUpdateData*>(data);
    
    // Load and scale the image
    GError *error = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(update_data->temp_file.c_str(), &error);
    
    if (pixbuf) {
        // Scale to 300x400 as specified
        int target_width = 300;
        int target_height = 400;
        
        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, target_width, target_height, GDK_INTERP_BILINEAR);
        
        if (scaled_pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(update_data->widget->get_art_image()), scaled_pixbuf);
            g_object_unref(scaled_pixbuf);
        }
        
        g_object_unref(pixbuf);
    } else {
        if (error) {
            g_error_free(error);
        }
    }
    
    delete update_data;
    return FALSE;
}


void SystemInfoWidget::start_art_loading() {
    // Load existing art image if available
    load_existing_art();
}

GtkWidget* SystemInfoWidget::get_art_image() {
    return art_image;
}

void SystemInfoWidget::load_existing_art() {
    // Try to load existing art from temp directory
    const char* home = g_get_home_dir();
    std::string temp_file = std::string(home ? home : "") + "/.config/Light/Art/current_art.jpg";
    
    GError *error = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(temp_file.c_str(), &error);
    
    if (pixbuf) {
        // Scale to 300x400 as specified
        int target_width = 300;
        int target_height = 400;
        
        GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, target_width, target_height, GDK_INTERP_BILINEAR);
        
        if (scaled_pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(art_image), scaled_pixbuf);
            g_object_unref(scaled_pixbuf);
        }
        
        g_object_unref(pixbuf);
    } else {
        // Clear image if no existing art found
        gtk_image_clear(GTK_IMAGE(art_image));
        if (error) {
            g_error_free(error);
        }
    }
}

void SystemInfoWidget::refresh_art_image() {
    // Start async image loading in background thread
    g_thread_new("safebooru_loader", [](gpointer user_data) -> gpointer {
        SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
        
        // Fetch random image URL from Safebooru
        std::string image_url = self->fetch_random_image_url();
        
        if (image_url.empty()) {
            // Update UI from main thread
            g_idle_add([](gpointer data) -> gboolean {
                SystemInfoWidget *widget = static_cast<SystemInfoWidget*>(data);
                gtk_image_clear(GTK_IMAGE(widget->art_image));
                return FALSE;
            }, self);
            return nullptr;
        }
        
        // Create temp directory for downloaded images
        const char* home = g_get_home_dir();
        std::string temp_dir = std::string(home ? home : "") + "/.config/Light/Art";
        std::string temp_file = temp_dir + "/current_art.jpg";
        
        // Create temp directory if it doesn't exist
        std::string mkdir_cmd = "mkdir -p " + temp_dir;
        system(mkdir_cmd.c_str());
        
        // Download the image
        if (self->download_image(image_url, temp_file)) {
            // Update UI from main thread
            ArtUpdateData *update_data = new ArtUpdateData{self, temp_file};
            g_idle_add(update_art_image_callback, update_data);
        }
        
        return nullptr;
    }, this);
}

gboolean SystemInfoWidget::on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->on_draw(widget, cr);
}

gboolean SystemInfoWidget::on_draw(GtkWidget *widget, cairo_t *cr) {
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

gboolean SystemInfoWidget::on_cpu_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->on_cpu_draw(widget, cr);
}

gboolean SystemInfoWidget::on_ram_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->on_ram_draw(widget, cr);
}

gboolean SystemInfoWidget::on_temp_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->on_temp_draw(widget, cr);
}

gboolean SystemInfoWidget::on_disk_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->on_disk_draw(widget, cr);
}

gboolean SystemInfoWidget::on_cpu_draw(GtkWidget *widget, cairo_t *cr) {
    return draw_circular_progress_with_hover(cr, cpu_usage, true, cpu_hovered);
}

gboolean SystemInfoWidget::on_ram_draw(GtkWidget *widget, cairo_t *cr) {
    return draw_circular_progress_with_hover(cr, ram_usage, false, ram_hovered);
}

gboolean SystemInfoWidget::on_temp_draw(GtkWidget *widget, cairo_t *cr) {
    return draw_circular_progress_for_widget_with_hover(cr, temp_usage, widget, "temp", temp_hovered);
}

gboolean SystemInfoWidget::on_disk_draw(GtkWidget *widget, cairo_t *cr) {
    return draw_circular_progress_for_widget_with_hover(cr, disk_usage, widget, "disk", disk_hovered);
}

gboolean SystemInfoWidget::draw_circular_progress_with_hover(cairo_t *cr, double percentage, bool is_cpu, bool hovered) {
    GtkAllocation allocation;
    GtkWidget *canvas = is_cpu ? cpu_canvas : ram_canvas;
    gtk_widget_get_allocation(canvas, &allocation);
    
    double center_x = allocation.width / 2.0;
    double center_y = allocation.height / 2.0;
    double base_radius = 30.0;
    double base_line_width = 6.0;
    
    // Use individual hover state instead of global
    double scale = hovered ? 1.15 : 1.0;
    double glow = hovered ? 1.0 : 0.0;
    
    // Apply scaling without clipping by using a smaller radius when scaled
    double radius = base_radius * (hovered ? 0.9 : 1.0); // Slightly smaller when hovered to prevent clipping
    double line_width = base_line_width;
    
    // Background circle with hover glow
    if (hovered && glow > 0) {
        // Glow effect - draw multiple circles for better glow
        for (int i = 3; i >= 1; i--) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1 * glow * i / 3.0);
            cairo_set_line_width(cr, line_width + i * 2);
            cairo_arc(cr, center_x, center_y, radius + i * 2, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }
    
    // Background circle
    cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
    cairo_set_line_width(cr, line_width);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Progress circle
    double progress = percentage / 100.0;
    double start_angle = -M_PI / 2; // Start from top
    double end_angle = start_angle + 2 * M_PI * progress;
    
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    if (is_cpu) {
        // CPU - blue gradient with enhanced hover effect
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
        if (g_use_hoc_theme) {
            double intensity = 0.8 + (0.2 * glow);
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, intensity);
        } else {
            double intensity = 0.8 + (0.2 * glow);
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, intensity);
        }
        cairo_set_source(cr, gradient);
        cairo_pattern_destroy(gradient);
    } else {
        // RAM - pink gradient with enhanced hover effect
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
        if (g_use_hoc_theme) {
            double intensity = 0.8 + (0.2 * glow);
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, intensity);
        } else {
            double intensity = 0.8 + (0.2 * glow);
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, intensity);
        }
        cairo_set_source(cr, gradient);
        cairo_pattern_destroy(gradient);
    }
    
    cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
    cairo_stroke(cr);
    
    return FALSE;
}

gboolean SystemInfoWidget::draw_circular_progress_for_widget_with_hover(cairo_t *cr, double percentage, GtkWidget *widget, const char* widget_type, bool hovered) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double center_x = allocation.width / 2.0;
    double center_y = allocation.height / 2.0;
    double base_radius = 30.0;
    double base_line_width = 6.0;
    
    // Use individual hover state instead of global
    double glow = hovered ? 1.0 : 0.0;
    
    // Apply scaling without clipping by using a smaller radius when scaled
    double radius = base_radius * (hovered ? 0.9 : 1.0); // Slightly smaller when hovered to prevent clipping
    double line_width = base_line_width;
    
    // Background circle with hover glow
    if (hovered && glow > 0) {
        // Glow effect - draw multiple circles for better glow
        for (int i = 3; i >= 1; i--) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1 * glow * i / 3.0);
            cairo_set_line_width(cr, line_width + i * 2);
            cairo_arc(cr, center_x, center_y, radius + i * 2, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }
    
    // Background circle
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1);
    cairo_set_line_width(cr, line_width);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Progress arc with different colors based on widget type
    double progress_angle = 2 * M_PI * (percentage / 100.0);
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
    
    if (strcmp(widget_type, "temp") == 0) {
        // Temperature - orange/red gradient with hover enhancement
        double intensity = 0.8 + (0.2 * glow);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, intensity);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, intensity);
        }
    } else if (strcmp(widget_type, "disk") == 0) {
        // Disk - green gradient with hover enhancement
        double intensity = 0.8 + (0.2 * glow);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, intensity);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, intensity);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, intensity);
        }
    }
    
    cairo_set_source(cr, gradient);
    cairo_arc(cr, center_x, center_y, radius, -M_PI/2, -M_PI/2 + progress_angle);
    cairo_stroke(cr);
    cairo_pattern_destroy(gradient);
    
    return FALSE;
}

gboolean SystemInfoWidget::draw_circular_progress_for_widget(cairo_t *cr, double percentage, GtkWidget *widget, const char* widget_type) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    double center_x = allocation.width / 2.0;
    double center_y = allocation.height / 2.0;
    double radius = 30.0;  // Back to original radius for 80x80 canvas
    double line_width = 6.0;  // Back to original line width
    
    // Background circle
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1);
    cairo_set_line_width(cr, line_width);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Progress arc with different colors based on widget type
    double progress_angle = 2 * M_PI * (percentage / 100.0);
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
    
    if (strcmp(widget_type, "temp") == 0) {
        // Temperature - orange/red gradient
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, 0.8);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, 0.8);
        }
    } else if (strcmp(widget_type, "disk") == 0) {
        // Disk - green gradient
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, 0.8);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, 0.8);
        }
    }
    
    cairo_set_source(cr, gradient);
    cairo_arc(cr, center_x, center_y, radius, -M_PI/2, -M_PI/2 + progress_angle);
    cairo_stroke(cr);
    cairo_pattern_destroy(gradient);
    
    return FALSE;
}

gboolean SystemInfoWidget::draw_circular_progress(cairo_t *cr, double percentage, bool is_cpu) {
    GtkAllocation allocation;
    GtkWidget *canvas = is_cpu ? cpu_canvas : ram_canvas;
    gtk_widget_get_allocation(canvas, &allocation);
    
    double center_x = allocation.width / 2.0;
    double center_y = allocation.height / 2.0;
    double radius = 30.0;  // Back to original radius for 80x80 canvas
    double line_width = 6.0;  // Back to original line width
    
    // Background circle
    cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
    cairo_set_line_width(cr, line_width);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    
    // Progress circle
    double progress = percentage / 100.0;
    double start_angle = -M_PI / 2; // Start from top
    double end_angle = start_angle + 2 * M_PI * progress;
    
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    if (is_cpu) {
        // CPU - blue gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, 0.8);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, 0.8);
        }
        cairo_set_source(cr, gradient);
        cairo_pattern_destroy(gradient);
    } else {
        // RAM - pink gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.439, 0.467, 0.741, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.694, 0.788, 0.925, 0.8);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.8, 0.2, 0.4, 0.8);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.4, 0.6, 0.8);
        }
        cairo_set_source(cr, gradient);
        cairo_pattern_destroy(gradient);
    }
    
    cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
    cairo_stroke(cr);
    
    return FALSE;
}

// Mouse event handlers
gboolean SystemInfoWidget::on_mouse_enter_static(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(g_object_get_data(G_OBJECT(widget), "system-info-widget"));
    if (self) {
        return self->on_mouse_enter(widget, event, (const char*)user_data);
    }
    return FALSE;
}

gboolean SystemInfoWidget::on_mouse_leave_static(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(g_object_get_data(G_OBJECT(widget), "system-info-widget"));
    if (self) {
        return self->on_mouse_leave(widget, event, (const char*)user_data);
    }
    return FALSE;
}

gboolean SystemInfoWidget::on_mouse_enter(GtkWidget *widget, GdkEventCrossing *event, const char* widget_type) {
    if (strcmp(widget_type, "cpu") == 0) {
        cpu_hovered = true;
    } else if (strcmp(widget_type, "ram") == 0) {
        ram_hovered = true;
    } else if (strcmp(widget_type, "temp") == 0) {
        temp_hovered = true;
    } else if (strcmp(widget_type, "disk") == 0) {
        disk_hovered = true;
    }
    
    start_animation();
    gtk_widget_queue_draw(widget);
    return FALSE;
}

gboolean SystemInfoWidget::on_mouse_leave(GtkWidget *widget, GdkEventCrossing *event, const char* widget_type) {
    if (strcmp(widget_type, "cpu") == 0) {
        cpu_hovered = false;
    } else if (strcmp(widget_type, "ram") == 0) {
        ram_hovered = false;
    } else if (strcmp(widget_type, "temp") == 0) {
        temp_hovered = false;
    } else if (strcmp(widget_type, "disk") == 0) {
        disk_hovered = false;
    }
    
    start_animation();
    gtk_widget_queue_draw(widget);
    return FALSE;
}

// Animation functions
gboolean SystemInfoWidget::animation_tick_static(gpointer user_data) {
    SystemInfoWidget *self = static_cast<SystemInfoWidget*>(user_data);
    return self->animation_tick();
}

gboolean SystemInfoWidget::animation_tick() {
    // Simple approach: just redraw the hovered widgets directly
    // No complex animation needed since we're using immediate hover states
    
    if (cpu_hovered) {
        gtk_widget_queue_draw(cpu_canvas);
    }
    if (ram_hovered) {
        gtk_widget_queue_draw(ram_canvas);
    }
    if (temp_hovered) {
        gtk_widget_queue_draw(temp_canvas);
    }
    if (disk_hovered) {
        gtk_widget_queue_draw(disk_canvas);
    }
    
    // Stop animation if no widgets are hovered
    if (!cpu_hovered && !ram_hovered && !temp_hovered && !disk_hovered) {
        animation_timer_id = 0;
        return FALSE; // Stop animation
    }
    
    return TRUE; // Continue animation
}

void SystemInfoWidget::start_animation() {
    if (animation_timer_id == 0) {
        animation_timer_id = g_timeout_add(16, animation_tick_static, this); // ~60 FPS
    }
}
