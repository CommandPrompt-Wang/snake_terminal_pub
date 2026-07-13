#pragma once
#include "scene/scene.h"
#include "game/snake.h"
#include "config/config.h"
#include "render/render.h"
#include "render/draw_list.h"
#include "render/sprite.h"
#include "game/snake_render.h"
#include "raylib.h"

#include <chrono>
#include <cmath>
#include <deque>
#include <list>
#include <memory>
#include <vector>

// -- GameScene --------------------------------------
// The main game scene. The game logic from input_thread.cpp runs here as a callback.
//
// Event-driven: instead of polling EventServer state variables,
// SceneManager constructs InputEvent and dispatches via handle_event().
class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() override;

    void on_enter() override;
    void on_exit() override;
    void update(float dt) override;
    void render() override;

    bool is_finished() const override { return finished_; }
    int get_next_scene_id() const override { return next_scene_id_; }
    const char* get_name() const override { return "GameScene"; }

protected:
    // -- Event handling --
    void on_inputevent(InputEvent& event) override;

private:
    // -- helpers --
    static bool is_opposite(Direction a, Direction b);
    static Position next_head(const SnakeState &s);
    static bool on_body(const std::deque<Position> &body, const Position &p, int skipFront = 0);
    void init_snake(SnakeState &s, int startX, int startY, Direction dir, int len);
    bool tick_player(SnakeState &p, int player, const SnakeState &other, Position &apple);
    void rebuild_snake_sprites();
    void consume_pending_dir();

    // -- state --
    SnakeState p1_, p2_;
    Position apple_{-1, -1};
    int score1_ = 0, score2_ = 0;
    int speed1_ = 1, speed2_ = 1;
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::DIE);

    // Direction buffer (set by on_inputevent, consumed in update)
    // Using optional-like flags for "no pending direction"
    bool has_pending_dir1_ = false;
    bool has_pending_dir2_ = false;
    Direction pending_dir1_ = Direction::DOWN;
    Direction pending_dir2_ = Direction::UP;

    // timing
    using Clock = std::chrono::steady_clock;
    Clock::time_point last_tick_;

    // rendering
    Draw_List draw_list_;
    std::vector<std::unique_ptr<Snake_Block>> snake_blocks_;

    // cell size for rendering
    static constexpr int CELL_SIZE = 30;
    static constexpr int OFFSET_X = 100;
    static constexpr int OFFSET_Y = 20;
};
