#pragma once

constexpr int GRID_W = 20;
constexpr int GRID_H = 20;

constexpr int CELL_SIZE = 32;
constexpr int OFFSET_X = 0;
constexpr int OFFSET_Y = 200;

inline float grid_to_pixel_x(int gx) { return static_cast<float>(gx * CELL_SIZE + OFFSET_X); }
inline float grid_to_pixel_y(int gy) { return static_cast<float>(gy * CELL_SIZE + OFFSET_Y); }

// O--------> x+
// |
// |
// v y+
struct Position {
    int x, y;
    bool operator==(const Position &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Position &o) const { return !(*this == o); }
};

enum class SceneId {
    MENU,
    CONFIG,
    GAME,
    DIE,
};

// -- Direction enum --
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
};
