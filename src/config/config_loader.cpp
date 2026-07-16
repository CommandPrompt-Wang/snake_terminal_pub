#include "config/config_loader.h"
#include "config/config.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static fs::path g_config_path;

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

static bool dir_is_writable(const fs::path& dir) {
    if (dir.empty()) return false;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return false;
#if defined(__linux__) || defined(__APPLE__)
    return access(dir.c_str(), W_OK) == 0;
#else
    fs::path probe = dir / ".snake_write_test";
    {
        std::ofstream f(probe);
        if (!f) return false;
    }
    fs::remove(probe, ec);
    return true;
#endif
}

static fs::path xdg_config_home() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdg) return fs::path(xdg);
    }
    if (const char* home = std::getenv("HOME"))
        return fs::path(home) / ".config";
    return {};
}

static fs::path xdg_config_file() {
    fs::path xdg = xdg_config_home();
    if (xdg.empty()) return {};
    fs::path dir = xdg / "snake_terminal";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (!dir_is_writable(dir)) return {};
    return dir / "snake.cfg";
}

// 若目录可写，返回该目录下的 snake.cfg（AppImage 文件路径则取其所在目录）
static fs::path portable_cfg_if_writable(const fs::path& base) {
    if (base.empty()) return {};
    fs::path dir = base;
    std::error_code ec;
    if (fs::is_regular_file(dir, ec))
        dir = dir.parent_path();
    if (dir.empty() || !dir_is_writable(dir))
        return {};
    return dir / "snake.cfg";
}

static fs::path resolve_config_path(const fs::path& app_dir) {
#if defined(__linux__) || defined(__APPLE__)
    // AppImage 运行时注入 APPIMAGE / ARGV0 → 磁盘上 .AppImage 所在目录（非只读挂载点）
    if (const char* appimage = std::getenv("APPIMAGE")) {
        if (fs::path cfg = portable_cfg_if_writable(appimage); !cfg.empty())
            return cfg;
    }
    if (const char* argv0 = std::getenv("ARGV0")) {
        if (fs::path cfg = portable_cfg_if_writable(argv0); !cfg.empty())
            return cfg;
    }
    if (const char* portable = std::getenv("SNAKE_PORTABLE_DIR")) {
        if (fs::path cfg = portable_cfg_if_writable(portable); !cfg.empty())
            return cfg;
    }
#endif

    if (!app_dir.empty()) {
        if (fs::path cfg = portable_cfg_if_writable(app_dir); !cfg.empty())
            return cfg;
    }

#if defined(__linux__) || defined(__APPLE__)
    if (fs::path xdg = xdg_config_file(); !xdg.empty())
        return xdg;
#endif

    return app_dir.empty() ? fs::path("snake.cfg") : app_dir / "snake.cfg";
}

void init_config_path(const fs::path& app_dir) {
    g_config_path = resolve_config_path(app_dir);
}

void log_config_startup() {
#if defined(__linux__)
    if (const char* appimage = std::getenv("APPIMAGE"); appimage && *appimage) {
        log("AppImage detected: " + std::string(appimage));
    } else if (const char* appdir = std::getenv("APPDIR"); appdir && *appdir) {
        log("AppImage detected (APPDIR=" + std::string(appdir) + ")");
    }
#endif
    if (!g_config_path.empty())
        log("config file: " + g_config_path.string());
}

const fs::path& config_path() {
    return g_config_path;
}

static bool ensure_config_parent() {
    fs::path parent = g_config_path.parent_path();
    if (parent.empty()) return true;
    std::error_code ec;
    fs::create_directories(parent, ec);
    return dir_is_writable(parent);
}

static bool load_config_file(const fs::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        if (fs::is_directory(path)) {
            logw("A Foldier?!! Ohh fxxk u! ('" + path.string() + "' is a directory)");
            return false;
        }
        log("config file '" + path.string() + "' not found or inaccessible, first launch?");
        log("creating one with default values: " + defaults_string());
        save_config();
        f.open(path);
        if (!f.is_open()) {
            logw("cannot create config, using defaults: " + defaults_string());
            return false;
        }
    }

    Config& cfg = game_config();
    std::string line;
    while (std::getline(f, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        auto key = line.substr(0, eq);
        auto val = line.substr(eq + 1);

        auto trim = [](std::string& s) {
            auto p = s.find_first_not_of(" \t\r\n");
            s.erase(0, p);
            p = s.find_last_not_of(" \t\r\n");
            if (p != std::string::npos) s.erase(p + 1);
        };
        trim(key);
        trim(val);

        bool  b = (val == "1" || val == "true" || val == "yes");
        float fv = 0.0f;
        int i = 0;
        try { fv = std::stof(val); } catch (...) { if (val != "true" && val != "false" && val != "yes" && val != "no") logw("failed to parse float: " + key + " = " + val); }
        try { i = std::stoi(val); } catch (...) { if (val != "true" && val != "false" && val != "yes" && val != "no") logw("failed to parse int: " + key + " = " + val); }

        if (key == "volume")                  cfg.volume                   = std::clamp(i, 0, 100);
        if (key == "allow_acceleration")        cfg.allowAcceleration        = b;
        if (key == "toroidal_space")            cfg.toroidalSpace            = b;
        if (key == "allow_through_others")      cfg.allowThroughOthers       = b;
        if (key == "speed_factor")              cfg.speed_factor             = std::max(0.1f, fv);
        if (key == "increasing_difficulty")     cfg.increasing_difficulty    = std::max(0.0f, fv);
        if (key == "time_match_duration")       cfg.time_match_duration      = std::max(0, i);
        if (key == "reborn_costs")               cfg.reborn_costs             = std::max(0, i);
        if (key == "respawn_in_advance")          cfg.respawnInAdvance         = b;
        if (key == "death_anim_interrupt")         cfg.deathAnimInterruptThreshold = std::max(-1, i);
    }
    return true;
}

bool load_config() {
    if (g_config_path.empty()) {
        logw("config path not initialized");
        return false;
    }
    return load_config_file(g_config_path);
}

void save_config() {
    if (g_config_path.empty()) {
        logw("config path not initialized");
        return;
    }
    if (!ensure_config_parent()) {
        logw("cannot create config directory: " + g_config_path.parent_path().string());
        return;
    }

    std::ofstream f(g_config_path);
    if (!f.is_open()) {
        logw("cannot write config file '" + g_config_path.string() + "'");
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
