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
// The main game scene, now using the Snake class for snake management.
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
    // -- helper --
    void consume_pending_dir();

    // -- state --
    Snake p1_{1}, p2_{2};
    Position apple_{-1, -1};
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::DIE);

    bool pause = false;

    // Direction queue (pooled input, consumed in update)
    std::deque<Direction> pending_dirs1_;
    std::deque<Direction> pending_dirs2_;

    // timing
    using Clock = std::chrono::steady_clock;
    Clock::time_point last_tick_;
    float time_elapsed_ = 0;

    // rendering
    Draw_List draw_list_;
    SnakeBody snake_body_1_{nullptr, 0};
    SnakeBody snake_body_2_{nullptr, 0};

    // cell size for rendering
    static constexpr int CELL_SIZE = 32;
    static constexpr int OFFSET_X = 0;
    static constexpr int OFFSET_Y = 200;
};
