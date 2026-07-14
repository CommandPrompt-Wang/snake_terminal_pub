#pragma once

struct Config {
    bool allowAcceleration   = true;   // speed boost when holding a direction
    bool toroidalSpace       = false;  // wrap around edges (Pac‑man style)
    bool allowThroughOthers = false; // can pass through the other player
    float speed_factor         = 1.0f;  // overall game speed multiplier (min 0.1)
    float increasing_difficulty = 1.0f; // difficulty ramp coefficient (0 = constant speed)
    int time_match_duration = 120; // time match duration in seconds (0 = infinite, for timerace mode)
    int reborn_costs = 1;          // points deducted on death in TIMERACE mode
    bool respawnInAdvance = false;  // respawn before pressing key and showing a shadow
    int deathAnimInterruptThreshold = 3;  // 死亡动画可打断阈值（身长>此值时允许跳过剩余动画）
};

inline Config& game_config() {
    static Config cfg;
    return cfg;
}
