#pragma once
#include <fstream>
#include <iomanip>
#include <string>
#include "config/config.h"

// simple text‑based config loader
// format: key = value  (one per line, '#' for comments)
// # snake_terminal config
// allow_acceleration      = true
// toroidal_space          = false
// allow_through_others = false

static bool load_config(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    Config &cfg = game_config();
    std::string line;
    while (std::getline(f, line)) {
        // strip comment
        auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);

        // find '='
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        auto key = line.substr(0, eq);
        auto val = line.substr(eq + 1);

        // trim whitespace
        auto trim = [](std::string &s) {
            auto p = s.find_first_not_of(" \t\r\n");
            s.erase(0, p);
            p = s.find_last_not_of(" \t\r\n");
            if (p != std::string::npos) s.erase(p + 1);
        };
        trim(key);
        trim(val);

        bool  b = (val == "1" || val == "true" || val == "yes");
        float f = 0.0f;
        try { f = std::stof(val); } catch (...) {}
        int i = 0;
        try { i = std::stoi(val); } catch (...) {}

        if (key == "allow_acceleration")        cfg.allowAcceleration        = b;
        if (key == "toroidal_space")            cfg.toroidalSpace            = b;
        if (key == "allow_through_others")      cfg.allowThroughOthers       = b;
        if (key == "speed_factor")              cfg.speed_factor             = std::max(0.1f, f);
        if (key == "increasing_difficulty")     cfg.increasing_difficulty    = std::max(0.0f, f);
        if (key == "time_match_duration")       cfg.time_match_duration      = std::max(0, i);
        if (key == "reborn_costs")               cfg.reborn_costs             = std::max(0, i);
        if (key == "respawn_in_advance")          cfg.respawnInAdvance         = b;
        if (key == "death_anim_interrupt")         cfg.deathAnimInterruptThreshold = std::max(0, i);
    }
    return true;
}

static void save_config(const std::string &path) {
    std::ofstream f(path);
    if (!f.is_open()) return;

    Config &cfg = game_config();
    f << "# snake_terminal config\n";
    f << "allow_acceleration      = " << (cfg.allowAcceleration      ? "true" : "false") << "\n";
    f << "toroidal_space          = " << (cfg.toroidalSpace          ? "true" : "false") << "\n";
    f << "allow_through_others = " << (cfg.allowThroughOthers  ? "true" : "false") << "\n";
    f << "speed_factor            = " << std::fixed << std::setprecision(1) << cfg.speed_factor << "\n";
    f << "increasing_difficulty   = " << std::fixed << std::setprecision(1) << cfg.increasing_difficulty << "\n";
    f << "time_match_duration     = " << cfg.time_match_duration << "\n";
    f << "reborn_costs            = " << cfg.reborn_costs << "\n";
    f << "respawn_in_advance       = " << (cfg.respawnInAdvance ? "true" : "false") << "\n";
    f << "death_anim_interrupt     = " << cfg.deathAnimInterruptThreshold << "\n";
}
