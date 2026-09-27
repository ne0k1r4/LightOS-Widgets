#include "light-common.h"

// Global theme detection (shared)
bool g_use_hoc_theme = false;
bool g_theme_detected = false;

// Theme detection helper
void detect_global_theme() {
    FILE* pipe = popen("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null", "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
        }
        pclose(pipe);
        
        // Remove quotes and whitespace
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
            result.pop_back();
        }
        if (!result.empty() && ((result.front() == '\'' && result.back() == '\'') || 
                               (result.front() == '"' && result.back() == '"'))) {
            result = result.substr(1, result.size() - 2);
        }
        // Match the active LightOS dark GTK theme.
        g_use_hoc_theme = (result == "LightOS-HoC");
    }
    g_theme_detected = true;
}

// Refresh theme detection (for runtime theme changes)
void refresh_global_theme() {
    g_theme_detected = false;
    detect_global_theme();
}

// Common utility function
std::string run_command(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }
    
    return result;
}
