#ifndef MUSIC_DAEMON_H
#define MUSIC_DAEMON_H

#include "light-common.h"
#include "music-widget.h"
#include "stats-widget.h"
#include "brightness-widget.h"
#include "volume-widget.h"
#include "mic-widget.h"

class MusicDaemon {
private:
    std::unique_ptr<MusicWidget> widget;
    std::unique_ptr<StatsWidget> stats_widget;
    std::unique_ptr<BrightnessWidget> brightness_widget;
    std::unique_ptr<VolumeWidget> volume_widget;
    std::unique_ptr<MicWidget> mic_widget;
    int socket_fd = -1;
    
public:
    MusicDaemon();
    ~MusicDaemon();
    bool setup_socket();
    void cleanup_socket();
    void handle_client_connections();
    void run();
};

// Signal handler for clean shutdown
extern MusicDaemon* g_daemon;
void signal_handler(int sig);
int daemon_main(int argc, char *argv[]);

#endif // MUSIC_DAEMON_H
