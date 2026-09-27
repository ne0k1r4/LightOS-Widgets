#include "music-daemon.h"

MusicDaemon::MusicDaemon() {
    widget = std::make_unique<MusicWidget>();
    stats_widget = std::make_unique<StatsWidget>();
    brightness_widget = std::make_unique<BrightnessWidget>();
    volume_widget = std::make_unique<VolumeWidget>();
    mic_widget = std::make_unique<MicWidget>();
}

MusicDaemon::~MusicDaemon() {
    cleanup_socket();
}

bool MusicDaemon::setup_socket() {
    // Remove existing socket
    unlink(SOCKET_PATH);
    
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(socket_fd);
        return false;
    }
    
    if (listen(socket_fd, 5) == -1) {
        perror("listen");
        close(socket_fd);
        return false;
    }
    
    // Set non-blocking
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);
    
    return true;
}

void MusicDaemon::cleanup_socket() {
    if (socket_fd != -1) {
        close(socket_fd);
        unlink(SOCKET_PATH);
    }
}

void MusicDaemon::handle_client_connections() {
    int client_fd = accept(socket_fd, nullptr, nullptr);
    if (client_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    char buffer[256];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string command(buffer);
        
        if (command == "show") {
            if (!widget->get_visible()) {
                widget->show();
                write(client_fd, "shown", 5);
            } else {
                widget->hide();
                write(client_fd, "hidden", 6);
            }
        } else if (command == "stats") {
            if (!stats_widget->get_visible()) {
                stats_widget->show();
                write(client_fd, "shown", 5);
            } else {
                stats_widget->hide();
                write(client_fd, "hidden", 6);
            }
        } else if (command == "brightness_up") {
            brightness_widget->increase_brightness();
            write(client_fd, "increased", 9);
        } else if (command == "brightness_down") {
            brightness_widget->decrease_brightness();
            write(client_fd, "decreased", 9);
        } else if (command == "volume_up") {
            volume_widget->increase_volume();
            write(client_fd, "increased", 9);
        } else if (command == "volume_down") {
            volume_widget->decrease_volume();
            write(client_fd, "decreased", 9);
        } else if (command == "volume_mute") {
            volume_widget->toggle_mute();
            write(client_fd, "toggled", 7);
        } else if (command.substr(0, 6) == "mic_up") {
            int value = 5; // default value
            if (command.length() > 6) {
                try {
                    value = std::stoi(command.substr(6));
                } catch (...) {
                    value = 5;
                }
            }
            mic_widget->increase_volume(value);
            write(client_fd, "increased", 9);
        } else if (command.substr(0, 8) == "mic_down") {
            int value = 5; // default value
            if (command.length() > 8) {
                try {
                    value = std::stoi(command.substr(8));
                } catch (...) {
                    value = 5;
                }
            }
            mic_widget->decrease_volume(value);
            write(client_fd, "decreased", 9);
        } else if (command == "mic_mute") {
            mic_widget->toggle_mute();
            write(client_fd, "toggled", 7);
        } else if (command == "status") {
            const char* status = widget->get_visible() ? "visible" : "hidden";
            write(client_fd, status, strlen(status));
        } else if (command == "quit") {
            write(client_fd, "quitting", 8);
            close(client_fd);
            gtk_main_quit();
            return;
        }
    }
    
    close(client_fd);
}

void MusicDaemon::run() {
    if (!setup_socket()) {
        std::cerr << "Failed to setup socket" << std::endl;
        return;
    }
    
    // Write PID file
    std::ofstream pid_file(PID_FILE);
    pid_file << getpid() << std::endl;
    pid_file.close();
    
    // Add socket monitoring to GTK main loop
    GIOChannel* channel = g_io_channel_unix_new(socket_fd);
    g_io_add_watch(channel, G_IO_IN, [](GIOChannel* source, GIOCondition condition, gpointer data) -> gboolean {
        MusicDaemon* daemon = static_cast<MusicDaemon*>(data);
        daemon->handle_client_connections();
        return TRUE;
    }, this);
    g_io_channel_unref(channel);
    
    // Start widget hidden initially
    gtk_main();
    
    // Cleanup
    unlink(PID_FILE);
}

// Signal handler for clean shutdown
MusicDaemon* g_daemon = nullptr;

void signal_handler(int sig) {
    if (g_daemon) {
        gtk_main_quit();
    }
}

int daemon_main(int argc, char *argv[]) {
    // Check if daemon is already running
    std::ifstream pid_file(PID_FILE);
    if (pid_file.good()) {
        int pid;
        pid_file >> pid;
        pid_file.close();
        
        if (kill(pid, 0) == 0) {
            std::cerr << "Daemon is already running with PID " << pid << std::endl;
            return 1;
        } else {
            // Remove stale PID file
            unlink(PID_FILE);
        }
    }
    
    gtk_init(&argc, &argv);
    
    // Setup signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    MusicDaemon daemon;
    g_daemon = &daemon;
    
    std::cout << "Music widget daemon started. PID: " << getpid() << std::endl;
    
    daemon.run();
    
    return 0;
}
