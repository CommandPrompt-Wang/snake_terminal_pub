#pragma once
#include <deque>
#include <random>
#include "event.h"

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

enum class Scene {
    MENU,
    CONFIG,
    GAME,
    DIE,
};

struct SnakeState {
    std::deque<Position> body;
    Event curDir = Event::DOWN;
    int curSpeed = 1;

    void reset() {
        body.clear();
        curDir = Event::DOWN;
        curSpeed = 1;
    }
};

// utility ────────────────────────────────────────

inline Position random_apple_pos(const SnakeState &p1, const SnakeState &p2) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dx(0, GRID_W - 1);
    std::uniform_int_distribution<int> dy(0, GRID_H - 1);

    for (int tries = 0; tries < 200; ++tries) {
        Position p{dx(rng), dy(rng)};
        bool on_snake = false;
        for (auto &seg : p1.body) if (seg == p) { on_snake = true; break; }
        if (!on_snake) for (auto &seg : p2.body) if (seg == p) { on_snake = true; break; }
        if (!on_snake) return p;
    }
    return {-1, -1}; // grid full
}
