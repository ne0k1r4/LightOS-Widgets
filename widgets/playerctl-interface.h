#ifndef PLAYERCTL_INTERFACE_H
#define PLAYERCTL_INTERFACE_H

#include "light-common.h"

class PlayerctlInterface {
public:
    struct TrackInfo {
        std::string title = "No Media Playing";
        std::string artist = "Unknown Artist";
        std::string album = "Unknown Album";
        std::string art_url = "";
        double duration = 0.0;
        double position = 0.0;
        std::string status = "Stopped";
        bool has_media = false;
    };
    
    PlayerctlInterface();
    TrackInfo get_metadata_snapshot();
    double get_position_seconds();
    void play_pause();
    void next_track();
    void previous_track();
    void set_position(double seconds);
    
private:
    std::string active_player;
    std::unordered_map<std::string, double> last_player_position_sec;
    
    struct PlayerSnapshot {
        std::string name;
        std::string status;
        double position_sec = 0.0;
        double duration_sec = 0.0;
        bool valid = false;
    };
 
    static std::string escape_single_quotes(const std::string &s);
    std::string make_playerctl_cmd(const std::string &subcommand);
    void refresh_active_player();
    double parse_duration_string(const std::string& duration_str);
    double parse_position_string(const std::string& position_str);
};

#endif // PLAYERCTL_INTERFACE_H

