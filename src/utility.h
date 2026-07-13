#pragma once

constexpr int GRID_W = 20;
constexpr int GRID_H = 20;

// O--------> x+
// |
// |
// v y+
struct Position {
    int x, y;
    bool operator==(const Position &o) const { return x == o.x && y == o.y; }
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
