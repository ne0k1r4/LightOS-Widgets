#include "music-daemon.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <signal.h>

#define SOCKET_PATH "/tmp/light_widget_daemon"
#define PID_FILE "/tmp/light_widget_daemon.pid"

bool is_daemon_running() {
    std::ifstream pid_file(PID_FILE);
    if (!pid_file.good()) {
        return false;
    }
    
    int pid;
    pid_file >> pid;
    pid_file.close();
    
    // Check if process exists
    return kill(pid, 0) == 0;
}

bool send_command_to_daemon(const std::string& command, std::string& response) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock_fd);
        return false;
    }
    
    if (write(sock_fd, command.c_str(), command.length()) == -1) {
        perror("write");
        close(sock_fd);
        return false;
    }
    
    char buffer[256];
    ssize_t bytes_read = read(sock_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        response = buffer;
    }
    
    close(sock_fd);
    return true;
}

int start_daemon() {
    // Check if daemon is already running
    if (is_daemon_running()) {
        std::cout << "Daemon is already running" << std::endl;
        return 0;
    }
    
    // Start daemon as separate process
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run daemon
        // Try system PATH first, then fallback to current directory
        execlp("light-widget-daemon", "light-widget-daemon", "--daemon", nullptr);
        // If execlp fails, try current directory as fallback
        execl("./light-widget-daemon", "light-widget-daemon", "--daemon", nullptr);
        perror("execl");
        exit(1);
    } else if (pid > 0) {
        // Parent process - wait a moment and check if daemon started
        sleep(1);
        if (is_daemon_running()) {
            std::cout << "Daemon started successfully" << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to start daemon" << std::endl;
            return 1;
        }
    } else {
        perror("fork");
        return 1;
    }
}

void print_usage() {
    std::cout << "Usage: light-widget-daemon [OPTIONS]\n"
              << "Options:\n"
              << "  (no options)  Start the daemon\n"
              << "  --show        Show/hide the widget (toggles)\n"
              << "  --stats       Show/hide the stats widget (toggles)\n"
              << "  --notif       Show/hide the notification widget (toggles)\n"
              << "  --brightness up   Increase brightness by 5%\n"
              << "  --brightness down Decrease brightness by 5%\n"
              << "  --volume up       Increase volume by 5%\n"
              << "  --volume down     Decrease volume by 5%\n"
              << "  --volume mute     Toggle volume mute\n"
              << "  --mic up [value]  Increase mic volume by value (default: 5)\n"
              << "  --mic down [value] Decrease mic volume by value (default: 5)\n"
              << "  --mic mute        Toggle mic mute\n"
              << "  --status      Get daemon status\n"
              << "  --quit        Stop the daemon\n"
              << "  --daemon      Run as daemon (internal use)\n"
              << "  --help        Show this help\n";
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // No arguments - start daemon
        return start_daemon();
    }
    
    std::string arg = argv[1];
    
    if (arg == "--help" || arg == "-h") {
        print_usage();
        return 0;
    }
    
    if (arg == "--daemon") {
        // Run as daemon
        return daemon_main(argc, argv);
    }
    
    if (arg == "--show") {
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string response;
        if (send_command_to_daemon("show", response)) {
            std::cout << "Widget " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--stats") {
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string response;
        if (send_command_to_daemon("stats", response)) {
            std::cout << "Stats Widget " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--notif") {
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string response;
        if (send_command_to_daemon("notif", response)) {
            std::cout << "Notification Widget " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--brightness") {
        if (argc < 3) {
            std::cerr << "Error: --brightness requires 'up' or 'down' argument" << std::endl;
            return 1;
        }
        
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string direction = argv[2];
        std::string command = "brightness_" + direction;
        std::string response;
        
        if (send_command_to_daemon(command, response)) {
            std::cout << "Brightness " << direction << " - " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--volume") {
        if (argc < 3) {
            std::cerr << "Error: --volume requires 'up', 'down', or 'mute' argument" << std::endl;
            return 1;
        }
        
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string action = argv[2];
        std::string command = "volume_" + action;
        std::string response;
        
        if (send_command_to_daemon(command, response)) {
            std::cout << "Volume " << action << " - " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--mic") {
        if (argc < 3) {
            std::cerr << "Error: --mic requires 'up', 'down', or 'mute' argument" << std::endl;
            return 1;
        }
        
        if (!is_daemon_running()) {
            std::cerr << "Error: Daemon is not running. Start it first with 'light-widget-daemon'" << std::endl;
            return 1;
        }
        
        std::string action = argv[2];
        std::string command;
        
        if (action == "up" || action == "down") {
            int value = 5; // default value
            if (argc >= 4) {
                try {
                    value = std::stoi(argv[3]);
                } catch (...) {
                    std::cerr << "Error: Invalid value '" << argv[3] << "'. Using default value 5." << std::endl;
                }
            }
            command = "mic_" + action + std::to_string(value);
        } else if (action == "mute") {
            command = "mic_mute";
        } else {
            std::cerr << "Error: --mic requires 'up', 'down', or 'mute' argument" << std::endl;
            return 1;
        }
        
        std::string response;
        if (send_command_to_daemon(command, response)) {
            std::cout << "Mic " << action << " - " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--status") {
        if (!is_daemon_running()) {
            std::cout << "Daemon is not running" << std::endl;
            return 1;
        }
        
        std::string response;
        if (send_command_to_daemon("status", response)) {
            std::cout << "Widget is " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    if (arg == "--quit") {
        if (!is_daemon_running()) {
            std::cout << "Daemon is not running" << std::endl;
            return 0;
        }
        
        std::string response;
        if (send_command_to_daemon("quit", response)) {
            std::cout << "Daemon " << response << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to communicate with daemon" << std::endl;
            return 1;
        }
    }
    
    std::cerr << "Unknown option: " << arg << std::endl;
    print_usage();
    return 1;
}
