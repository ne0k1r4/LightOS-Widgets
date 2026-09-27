#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <signal.h>

#define SOCKET_PATH "/tmp/music_widget_daemon"
#define PID_FILE "/tmp/music_widget_daemon.pid"

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

// Forward declaration of daemon main function
int daemon_main(int argc, char* argv[]);

int start_daemon() {
    // Check if daemon is already running
    if (is_daemon_running()) {
        std::cout << "Daemon is already running" << std::endl;
        return 0;
    }
    
    // Fork and run daemon in background
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run daemon
        char* daemon_argv[] = {(char*)"music_widget", (char*)"--daemon", nullptr};
        return daemon_main(2, daemon_argv);
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
    std::cout << "Usage: music_widget [OPTIONS]\n"
              << "Options:\n"
              << "  (no options)  Start the daemon\n"
              << "  --show        Show/hide the widget (toggles)\n"
              << "  --stats       Show/hide the stats widget (toggles)\n"
              << "  --notif       Show/hide the notification widget (toggles)\n"
              << "  --status      Get daemon status\n"
              << "  --quit        Stop the daemon\n"
              << "  --daemon      Run as daemon (internal use)\n"
              << "  --help        Show this help\n";
}

int main(int argc, char* argv[]) {
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
            std::cerr << "Error: Daemon is not running. Start it first with 'music_widget'" << std::endl;
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
            std::cerr << "Error: Daemon is not running. Start it first with 'music_widget'" << std::endl;
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
            std::cerr << "Error: Daemon is not running. Start it first with 'music_widget'" << std::endl;
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
