#ifndef LIGHT_COMMON_H
#define LIGHT_COMMON_H

#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <cairo.h>
#include <gdk/gdk.h>
#include <cmath>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <vector>
#include <array>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <ctime>
#include <fcntl.h>
#include <json/json.h>
#include <sys/statvfs.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

// Socket path for IPC
#define SOCKET_PATH "/tmp/light_widget_daemon"
#define PID_FILE "/tmp/light_widget_daemon.pid"

// Forward declarations
class CalendarWidget;
class MusicWidget;
class RecentAppsWidget;
class SystemInfoWidget;
class StatsWidget;
class BrightnessWidget;
class VolumeWidget;
class MicWidget;

// Global theme detection (shared)
extern bool g_use_hoc_theme;
extern bool g_theme_detected;

// Theme detection functions
void detect_global_theme();
void refresh_global_theme();

// Common utility functions
std::string run_command(const std::string& cmd);

#endif // LIGHT_COMMON_H
