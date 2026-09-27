#include "playerctl-interface.h"

PlayerctlInterface::PlayerctlInterface() {
    // defer active player discovery until first query
}

PlayerctlInterface::TrackInfo PlayerctlInterface::get_metadata_snapshot() {
    TrackInfo info;
    
    std::string fmt = "metadata --format '{{playerName}}|{{status}}|{{title}}|{{artist}}|{{album}}|{{mpris:length}}|{{position}}|{{mpris:artUrl}}' 2>/dev/null";
    std::string combined = run_command(std::string("playerctl ") + fmt);
    if (combined.empty()) {
        info.has_media = false;
        return info;
    }
    
    std::array<std::string, 8> parts{};
    size_t start = 0; int idx = 0;
    while (idx < 8) {
        size_t pos = combined.find('|', start);
        std::string token = (pos == std::string::npos) ? combined.substr(start) : combined.substr(start, pos - start);
        parts[idx++] = token;
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
    for (; idx < 8; ++idx) parts[idx] = "";
    
    if (!parts[0].empty()) active_player = parts[0];
    info.status = parts[1];
    info.title = parts[2];
    info.artist = parts[3];
    info.album = parts[4];
    
    if (!parts[5].empty()) {
        try {
            double us = std::stod(parts[5]);
            if (us > 0) info.duration = us / 1000000.0;
        } catch (...) {}
    }
    if (!parts[6].empty()) {
        try {
            double us = std::stod(parts[6]);
            if (us >= 0) info.position = us / 1000000.0;
        } catch (...) {}
    }
    
    if (info.duration <= 0) {
        std::string duration_fmt = run_command(make_playerctl_cmd("metadata --format '{{ duration(mpris:length) }}' 2>/dev/null"));
        if (!duration_fmt.empty() && duration_fmt.find("{{") == std::string::npos) {
            info.duration = parse_duration_string(duration_fmt);
        }
    }
    
    info.art_url = parts[7];
    if (!info.art_url.empty() && info.art_url != "{{ mpris:artUrl }}") {
        if (info.art_url.substr(0, 7) == "file://") {
            info.art_url = info.art_url.substr(7);
        }
    } else {
        info.art_url = "";
    }
    
    if (info.title.empty() || info.title == "{{ title }}") info.title = "Unknown Title";
    if (info.artist.empty() || info.artist == "{{ artist }}") info.artist = "Unknown Artist";
    if (info.album.empty() || info.album == "{{ album }}") info.album = "Unknown Album";
    
    if (info.duration < 0) info.duration = 0;
    if (info.position < 0) info.position = 0;
    if (info.duration > 0 && info.position > info.duration + 1.0) {
        info.position = info.duration;
    }
    
    info.has_media = !info.status.empty() && info.status != "No players found";
    
    return info;
}

double PlayerctlInterface::get_position_seconds() {
    std::string position_str;
    if (!active_player.empty()) {
        position_str = run_command(make_playerctl_cmd("position 2>/dev/null"));
    } else {
        position_str = run_command("playerctl position 2>/dev/null");
    }
    if (!position_str.empty()) {
        return parse_position_string(position_str);
    }
    return -1.0;
}

void PlayerctlInterface::play_pause() {
    run_command(make_playerctl_cmd("play-pause 2>/dev/null"));
}

void PlayerctlInterface::next_track() {
    run_command(make_playerctl_cmd("next 2>/dev/null"));
}

void PlayerctlInterface::previous_track() {
    run_command(make_playerctl_cmd("previous 2>/dev/null"));
}

void PlayerctlInterface::set_position(double seconds) {
    std::string cmd = make_playerctl_cmd(std::string("position ") + std::to_string(seconds) + " 2>/dev/null");
    run_command(cmd);
}

std::string PlayerctlInterface::escape_single_quotes(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') out += "'\\''"; else out += c;
    }
    return out;
}

std::string PlayerctlInterface::make_playerctl_cmd(const std::string &subcommand) {
    if (active_player.empty()) {
        return std::string("playerctl ") + subcommand;
    }
    return std::string("playerctl -p '") + escape_single_quotes(active_player) + "' " + subcommand;
}

void PlayerctlInterface::refresh_active_player() {
    std::string list = run_command("playerctl -l 2>/dev/null");
    if (list.empty()) {
        active_player.clear();
        return;
    }
    std::vector<std::string> players;
    {
        std::stringstream ss(list);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty()) players.push_back(line);
        }
    }
    
    std::vector<PlayerSnapshot> snapshots;
    snapshots.reserve(players.size());
    for (const auto &p : players) {
        PlayerSnapshot snap;
        snap.name = p;
        std::string resp = run_command(std::string("playerctl -p '") + escape_single_quotes(p) + "' metadata --format '{{status}}|{{position}}|{{mpris:length}}' 2>/dev/null");
        if (resp.empty()) { snapshots.push_back(snap); continue; }
        size_t a = resp.find('|');
        size_t b = (a == std::string::npos) ? std::string::npos : resp.find('|', a + 1);
        std::string st = (a == std::string::npos) ? resp : resp.substr(0, a);
        std::string pos_us = (a == std::string::npos || b == std::string::npos) ? std::string("") : resp.substr(a + 1, b - a - 1);
        std::string len_us = (b == std::string::npos) ? std::string("") : resp.substr(b + 1);
        snap.status = st;
        try { if (!pos_us.empty()) snap.position_sec = std::stod(pos_us) / 1000000.0; } catch (...) { snap.position_sec = 0.0; }
        try { if (!len_us.empty()) snap.duration_sec = std::stod(len_us) / 1000000.0; } catch (...) { snap.duration_sec = 0.0; }
        snap.valid = true;
        snapshots.push_back(snap);
    }
    
    std::string previous = active_player;
    if (!previous.empty()) {
        for (const auto &s : snapshots) {
            if (s.name == previous && (s.status == "Playing" || s.status == "Paused")) {
                last_player_position_sec[s.name] = s.position_sec;
                return;
            }
        }
    }
    
    std::vector<PlayerSnapshot> playing;
    for (const auto &s : snapshots) if (s.valid && s.status == "Playing") playing.push_back(s);
    
    if (playing.size() == 1) {
        active_player = playing[0].name;
        last_player_position_sec[active_player] = playing[0].position_sec;
        return;
    }
    
    if (playing.size() > 1) {
        double best_delta = -1e9;
        std::string best;
        for (const auto &s : playing) {
            double last = (last_player_position_sec.count(s.name) ? last_player_position_sec[s.name] : s.position_sec);
            double delta = s.position_sec - last;
            if (delta < 0) delta = 1.0;
            if (delta > best_delta) { best_delta = delta; best = s.name; }
        }
        if (!best.empty()) {
            active_player = best;
            last_player_position_sec[active_player] = 0.0;
            return;
        }
    }
    
    for (const auto &s : snapshots) {
        if (s.valid && s.status == "Paused") {
            active_player = s.name;
            last_player_position_sec[active_player] = s.position_sec;
            return;
        }
    }
    
    for (const auto &s : snapshots) {
        if (s.valid) {
            active_player = s.name;
            last_player_position_sec[active_player] = s.position_sec;
            return;
        }
    }
    
    active_player.clear();
}

double PlayerctlInterface::parse_duration_string(const std::string& duration_str) {
    std::regex time_regex(R"((?:(\d+):)?(\d+):(\d+))");
    std::smatch matches;
    
    if (std::regex_search(duration_str, matches, time_regex)) {
        double total_seconds = 0.0;
        
        if (matches[1].matched) {
            total_seconds += std::stod(matches[1]) * 3600;
        }
        
        total_seconds += std::stod(matches[2]) * 60;
        total_seconds += std::stod(matches[3]);
        
        return total_seconds;
    }
    
    try {
        return std::stod(duration_str);
    } catch (...) {
        return 0.0;
    }
}

double PlayerctlInterface::parse_position_string(const std::string& position_str) {
    double from_colon = parse_duration_string(position_str);
    if (from_colon > 0.0) {
        return from_colon;
    }
    
    std::string cleaned;
    cleaned.reserve(position_str.size());
    for (char c : position_str) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ',') {
            cleaned.push_back(c);
        } else if (std::isspace(static_cast<unsigned char>(c)) || std::isalpha(static_cast<unsigned char>(c))) {
            break;
        }
    }
    if (cleaned.empty()) {
        return 0.0;
    }
    std::replace(cleaned.begin(), cleaned.end(), ',', '.');
    try {
        return std::stod(cleaned);
    } catch (...) {
        return 0.0;
    }
}

