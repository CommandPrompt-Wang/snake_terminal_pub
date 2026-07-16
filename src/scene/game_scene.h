#pragma once
#include "scene/scene.h"
#include "game/snake.h"
#include "config/config.h"
#include "render/render.h"
#include "render/draw_list.h"
#include "render/sprite.h"
#include "game/snake_render.h"
#include "game/snake_anim.h"
#include "raylib.h"

#include <cmath>
#include <deque>
#include <memory>

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
    void on_inputevent(InputEvent& event) override;

private:
    void consume_pending_dir();
    void setup_snake_body(std::unique_ptr<SnakeBody>& body, Snake& snake, int playerId);
    void on_die_finished_handler(Snake& snake);
    bool try_interrupt_dying(SnakeBody& body);
    bool handle_respawn(Snake& s, Snake& otherSnake, Direction dir,
                        std::deque<Direction>& pending,
                        SnakeBody& body, SnakeBody& otherBody);
    void handle_move_key(Snake& s, Snake& otherSnake, Direction dir,
                         std::deque<Direction>& pending,
                         SnakeBody& body, SnakeBody& otherBody);
    void sync_scores_and_finish();

    Sprite apple{"resources/img/apple.png", 10};
    Snake p1_{1}, p2_{2};
    Position apple_{-1, -1};
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::DIE);
    bool pause = false;

    std::deque<Direction> pending_dirs1_;
    std::deque<Direction> pending_dirs2_;

    float tick_remain1_ = 0;
    float tick_remain2_ = 0;
    float time_elapsed_ = 0;
    int time_remaining_ = 0;
    float last_tick_sec_ = 0.3f;

    Draw_List draw_list_;
    std::unique_ptr<SnakeBody> snake_body_1_;
    std::unique_ptr<SnakeBody> snake_body_2_;
};
