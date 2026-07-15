#include "config/config_loader.h"
#include "config/config.h"
#include "utils/log.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

static std::string defaults_string() {
    const auto& c = game_config();
    std::ostringstream s;
    s << "volume=" << c.volume
      << " accel=" << (c.allowAcceleration ? "true" : "false")
      << " toroidal=" << (c.toroidalSpace ? "true" : "false")
      << " through=" << (c.allowThroughOthers ? "true" : "false")
      << " speed=" << std::fixed << std::setprecision(1) << c.speed_factor
      << " diff=" << c.increasing_difficulty
      << " time=" << c.time_match_duration
      << " reborn=" << c.reborn_costs
      << " ghost=" << (c.respawnInAdvance ? "true" : "false")
      << " interrupt=" << c.deathAnimInterruptThreshold;
    return s.str();
}

bool load_config(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        if (std::filesystem::is_directory(path)) {
            logw("A Foldier?!! Ohh fxxk u! ('" + path + "' is a directory)");
            return false;
        }
        log("config file '" + path + "' not found or inaccessible, first launch?");
        log("creating one with default values: " + defaults_string());
        save_config(path);  // 尝试用默认值创建
        f.open(path);
        if (!f.is_open()) {
            logw("cannot create config, using defaults: " + defaults_string());
            return false;
        }
    }

    Config& cfg = game_config();
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
        auto trim = [](std::string& s) {
            auto p = s.find_first_not_of(" \t\r\n");
            s.erase(0, p);
            p = s.find_last_not_of(" \t\r\n");
            if (p != std::string::npos) s.erase(p + 1);
        };
        trim(key);
        trim(val);

        bool  b = (val == "1" || val == "true" || val == "yes");
        float f = 0.0f;
        int i = 0;
        try { f = std::stof(val); } catch (...) { if (val != "true" && val != "false" && val != "yes" && val != "no") logw("failed to parse float: " + key + " = " + val); }
        try { i = std::stoi(val); } catch (...) { if (val != "true" && val != "false" && val != "yes" && val != "no") logw("failed to parse int: " + key + " = " + val); }

        if (key == "volume")                  cfg.volume                   = std::clamp(i, 0, 100);
        if (key == "allow_acceleration")        cfg.allowAcceleration        = b;
        if (key == "toroidal_space")            cfg.toroidalSpace            = b;
        if (key == "allow_through_others")      cfg.allowThroughOthers       = b;
        if (key == "speed_factor")              cfg.speed_factor             = std::max(0.1f, f);
        if (key == "increasing_difficulty")     cfg.increasing_difficulty    = std::max(0.0f, f);
        if (key == "time_match_duration")       cfg.time_match_duration      = std::max(0, i);
        if (key == "reborn_costs")               cfg.reborn_costs             = std::max(0, i);
        if (key == "respawn_in_advance")          cfg.respawnInAdvance         = b;
        if (key == "death_anim_interrupt")         cfg.deathAnimInterruptThreshold = std::max(-1, i);
    }
    return true;
}

void save_config(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) {
        logw("cannot write config file '" + path + "'");
        return;
    }

    const Config& cfg = game_config();
    f << "# snake_terminal config\n\n";
    f << "# 0-100 linear\n";
    f << "volume                  = " << cfg.volume << "\n\n";
    f << "# Press L/R Shift to double the speed\n";
    f << "allow_acceleration      = " << (cfg.allowAcceleration      ? "true" : "false") << "\n\n";
    f << "# Allow passing through walls\n";
    f << "toroidal_space          = " << (cfg.toroidalSpace          ? "true" : "false") << "\n\n";
    f << "# Allow passing through the other player\n";
    f << "allow_through_others    = " << (cfg.allowThroughOthers     ? "true" : "false") << "\n\n";
    f << "# Overall game speed multiplier (min 0.1)\n";
    f << "speed_factor            = " << std::fixed << std::setprecision(1) << cfg.speed_factor << "\n\n";
    f << "# Difficulty ramp coefficient (0 = constant speed)\n";
    f << "increasing_difficulty   = " << std::fixed << std::setprecision(1) << cfg.increasing_difficulty << "\n\n";
    f << "# Time match duration in seconds for TIMERACE (0 = infinite)\n";
    f << "time_match_duration     = " << cfg.time_match_duration << "\n\n";
    f << "# Points deducted on death in TIMERACE mode\n";
    f << "reborn_costs            = " << cfg.reborn_costs << "\n\n";
    f << "# Respawn before pressing key, shows a ghost preview\n";
    f << "respawn_in_advance      = " << (cfg.respawnInAdvance       ? "true" : "false") << "\n\n";
    f << "# Death animation skip threshold, -1 = off\n";
    f << "death_anim_interrupt    = " << cfg.deathAnimInterruptThreshold << "\n\n";
}
