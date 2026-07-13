#pragma once

struct Config {
    bool allowAcceleration   = true;   // speed boost when holding a direction
    bool toroidalSpace       = false;  // wrap around edges (Pac‑man style)
    bool allowThroughOthers = false; // can pass through the other player
    float speed_factor         = 1.0f;  // overall game speed multiplier (min 0.1)
    float increasing_difficulty = 1.0f; // difficulty ramp coefficient (0 = constant speed)
};

inline Config& game_config() {
    static Config cfg;
    return cfg;
}
