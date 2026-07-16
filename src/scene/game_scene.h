#pragma once
#include "scene/scene.h"
#include "game/snake.h"
#include "config/config.h"
#include "render/render.h"
#include "render/drawlist.h"
#include "render/sprite.h"
#include "game/snake_render.h"
#include "game/snake_anim.h"
#include "raylib.h"

#include <cmath>
#include <deque>
#include <memory>

// 双人对战主场景：处理输入、蛇 tick、碰撞判定、死亡/复活与 HUD 渲染
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
    void consume_pending_dir();          // 将按键队列转为蛇方向（过滤 180° 反向）
    void setup_snake_body(std::unique_ptr<SnakeBody>& body, Snake& snake, int playerId);
    void on_die_finished_handler(Snake& snake);  // TIMERACE 模式：扣命/生成幽灵
    bool try_interrupt_dying(SnakeBody& body);   // 死亡动画中按键提前结束
    bool handle_respawn(Snake& s, Snake& otherSnake, Direction dir,
                        std::deque<Direction>& pending,
                        SnakeBody& body, SnakeBody& otherBody);
    void handle_move_key(Snake& s, Snake& otherSnake, Direction dir,
                         std::deque<Direction>& pending,
                         SnakeBody& body, SnakeBody& otherBody);
    void sync_scores_and_finish();

    Sprite apple{"resources/img/apple.png", 10};  // layer=10，绘制在蛇身之上
    Snake p1_{1}, p2_{2};       // 逻辑状态（网格坐标、分数、方向）
    Position apple_{-1, -1};
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::DIE);
    bool pause = false;

    std::deque<Direction> pending_dirs1_;  // 玩家输入缓冲，每 tick 消费一条
    std::deque<Direction> pending_dirs2_;

    float tick_remain1_ = 0;   // 距下次移动 tick 的剩余时间（秒）
    float tick_remain2_ = 0;
    float time_elapsed_ = 0;   // TIMERACE 已用时间
    int time_remaining_ = 0;
    float last_tick_sec_ = 0.3f;  // 最近一次 tick 间隔，用于同步死亡动画

    DrawList draw_list_;  // 蛇身等 BasicRenderClass 对象统一在此 update/draw
    std::unique_ptr<SnakeBody> snake_body_1_;
    std::unique_ptr<SnakeBody> snake_body_2_;
};
