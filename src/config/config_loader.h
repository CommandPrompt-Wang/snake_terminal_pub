#pragma once
#include <fstream>
#include <string>
#include "config/config.h"

// simple text‑based config loader
// format: key = value  (one per line, '#' for comments)
// # snake_terminal config
// allow_acceleration      = true
// toroidal_space          = false
// allow_through_teammates = false

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

        bool b = (val == "1" || val == "true" || val == "yes");
        int  n = 0;
        try { n = std::stoi(val); } catch (...) {}

        if (key == "allow_acceleration")        cfg.allowAcceleration      = b;
        if (key == "toroidal_space")            cfg.toroidalSpace          = b;
        if (key == "allow_through_teammates")   cfg.allowThroughTeammates  = b;
        if (key == "increasing_difficulty")     cfg.increasing_difficulty  = n;
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
    f << "allow_through_teammates = " << (cfg.allowThroughTeammates  ? "true" : "false") << "\n";
    f << "increasing_difficulty   = " << cfg.increasing_difficulty << "\n";
}
