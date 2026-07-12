#pragma once

struct Config {
    bool allowAcceleration   = true;   // speed boost when holding a direction
    bool toroidalSpace       = false;  // wrap around edges (Pac‑man style)
    bool allowThroughTeammates = false; // can pass through the other player
    int  increasing_difficulty = 1;    // difficulty ramp coefficient (0 = constant speed)
};

inline Config& game_config() {
    static Config cfg;
    return cfg;
}
