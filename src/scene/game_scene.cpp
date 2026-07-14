#include "scene/game_scene.h"
#include "config/config.h"
#include "global.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <iostream>

// -- consume buffered direction into actual snake dir --
void GameScene::consume_pending_dir() {
    while (!pending_dirs1_.empty()) {
        Direction d = pending_dirs1_.front();
        pending_dirs1_.pop_front();
        if (!Snake::is_opposite(p1_.get_last_move_dir(), d)) {
            p1_.set_direction(d);
        }
    }
    while (!pending_dirs2_.empty()) {
        Direction d = pending_dirs2_.front();
        pending_dirs2_.pop_front();
        if (!Snake::is_opposite(p2_.get_last_move_dir(), d)) {
            p2_.set_direction(d);
        }
    }
}

// -- constructor / destructor ---------------------

GameScene::GameScene()  = default;
GameScene::~GameScene() = default;

void GameScene::on_enter() {
    pause = false;
    finished_ = false;
    next_scene_id_ = static_cast<int>(SceneId::DIE);
    pending_dirs1_.clear();
    pending_dirs2_.clear();

    // 重置全局状态
    Global::player_status1 = Global::PlayerStatus::ALIVE;
    Global::player_status2 = Global::PlayerStatus::ALIVE;
    Global::end_reason = Global::GameOverReason::NONE;

    p1_.init(5, 5, Direction::DOWN, 3);
    p2_.init(GRID_W - 6, GRID_H - 6, Direction::UP, 3);
    p1_.set_score(0);
    p2_.set_score(0);
    p1_.set_animation_status(Snake::AnimationStatus::MOVE);
    p2_.set_animation_status(Snake::AnimationStatus::MOVE);

    apple_ = random_apple_pos(p1_, p2_);
    time_elapsed_ = 0;

    last_tick_ = Clock::now();
    
    snake_body_1_ = SnakeBody(&p1_, 1);
    snake_body_1_.set_scale({2.0,2.0});
    draw_list_.push_back(&snake_body_1_);
    snake_body_2_ = SnakeBody(&p2_, 2);
    snake_body_2_.set_scale({2.0,2.0});
    draw_list_.push_back(&snake_body_2_);
}

void GameScene::on_exit() {
    snake_body_1_.print_pos();
    snake_body_2_.print_pos();
    draw_list_.clear();
}

// -- Event handling ---------------------------------
// SceneManager constructs InputEvent and dispatches it here via handle_event()
void GameScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    // 尝试按键重生：WAITING 状态的蛇按下移动键时复活
    auto handle_respawn = [&](Snake& s, const Snake& other,
                              Direction dir, std::deque<Direction>& pending) -> bool {
        if (s.get_animation_status() != Snake::AnimationStatus::WAITING)
            return false; // 不由本 handler 处理

        if (!s.respawn(other)) {
            // 饿死 → 游戏结束
            Global::end_reason = Global::GameOverReason::DEATH;
            Global::last_score_player1 = p1_.get_score();
            Global::last_score_player2 = p2_.get_score();
            finished_ = true;
        } else {
            s.set_animation_status(Snake::AnimationStatus::MOVE);
            pending.push_back(dir); // 把该方向记入输入缓冲
        }
        return true; // 已消费
    };

    switch (event.get_key_code()) {
        // -- P1 移动键（WASD）--
        case KEY_W:
            if (handle_respawn(p1_, p2_, Direction::UP, pending_dirs1_)) break;
            pending_dirs1_.push_back(Direction::UP);
            break;
        case KEY_S:
            if (handle_respawn(p1_, p2_, Direction::DOWN, pending_dirs1_)) break;
            pending_dirs1_.push_back(Direction::DOWN);
            break;
        case KEY_A:
            if (handle_respawn(p1_, p2_, Direction::LEFT, pending_dirs1_)) break;
            pending_dirs1_.push_back(Direction::LEFT);
            break;
        case KEY_D:
            if (handle_respawn(p1_, p2_, Direction::RIGHT, pending_dirs1_)) break;
            pending_dirs1_.push_back(Direction::RIGHT);
            break;
        // -- P2 移动键（IJKL）--
        case KEY_I:
            if (handle_respawn(p2_, p1_, Direction::UP, pending_dirs2_)) break;
            pending_dirs2_.push_back(Direction::UP);
            break;
        case KEY_K:
            if (handle_respawn(p2_, p1_, Direction::DOWN, pending_dirs2_)) break;
            pending_dirs2_.push_back(Direction::DOWN);
            break;
        case KEY_J:
            if (handle_respawn(p2_, p1_, Direction::LEFT, pending_dirs2_)) break;
            pending_dirs2_.push_back(Direction::LEFT);
            break;
        case KEY_L:
            if (handle_respawn(p2_, p1_, Direction::RIGHT, pending_dirs2_)) break;
            pending_dirs2_.push_back(Direction::RIGHT);
            break;
        
        // -- 菜单 --
        case KEY_T:
            if (pause) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::MENU);
            }
            event.consume();
            break;
        case KEY_BACKSPACE:
        case KEY_ESCAPE:
            pause = !pause;
            event.consume();
            break;

        // -- 手动结束 --
        case KEY_Y:
            if (pause) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::DIE);
                Global::end_reason = Global::GameOverReason::MANUAL;
                Global::player_status1 = Global::PlayerStatus::ALIVE;
                Global::player_status2 = Global::PlayerStatus::ALIVE;
                event.consume();
            }
    }
}

void GameScene::update(float dt) {
    if (pause) {
        draw_list_.update();
        return;
    }

    time_elapsed_ += dt;

    // 1. 解析 WAITING 状态（由 SnakeBody 渲染层管理 DYING -> WAITING）
    //    - DEATHMATCH: 双蛇 WAITING → 结束（活的蛇已在 tick 后被设为 WAITING）
    //    - TIMERACE:   等待玩家按键重生，这里不做任何操作
    if (Global::last_game_mode == Global::GameMode::DEATHMATCH &&
        p1_.get_animation_status() == Snake::AnimationStatus::WAITING &&
        p2_.get_animation_status() == Snake::AnimationStatus::WAITING) {
        Global::end_reason = Global::GameOverReason::DEATH;
        Global::last_score_player1 = p1_.get_score();
        Global::last_score_player2 = p2_.get_score();
        finished_ = true;
    }

    if (finished_) {
        draw_list_.update();
        return;
    }

    // 2. 输入 + 加速（仅 MOVE 状态的蛇）
    consume_pending_dir();

    if (game_config().allowAcceleration) {
        if (p1_.get_animation_status() == Snake::AnimationStatus::MOVE)
            p1_.set_speed(IsKeyDown(KEY_LEFT_SHIFT) ? 2 : 1);
        if (p2_.get_animation_status() == Snake::AnimationStatus::MOVE)
            p2_.set_speed(IsKeyDown(KEY_RIGHT_SHIFT) ? 2 : 1);
    }

    // 3. Tick 定时
    int highScore = std::max((int)p1_.get_body().size() - 3, (int)p2_.get_body().size() - 3);
    double diffMult = 1.0;
    if (game_config().increasing_difficulty > 0) {
        double k = game_config().increasing_difficulty;
        diffMult = 1.0 + 2.0 * std::log10((k * highScore) / 9.0 + 1.0);
    }
    float spd = std::max(0.1f, game_config().speed_factor);
    double tickMs = 500.0 / (diffMult * spd);
    auto now = Clock::now();
    auto interval = std::chrono::milliseconds(static_cast<int>(tickMs));

    if (now - last_tick_ >= interval) {
        last_tick_ = now;

        // -- 仅 MOVE 状态的蛇参与 tick --
        // 先收集死亡结果，双方 tick 完成后再统一处理
        bool died1 = false, died2 = false;

        // P1 tick
        if (p1_.get_animation_status() == Snake::AnimationStatus::MOVE) {
            for (int i = 0; i < std::max(p1_.get_speed(), 1); ++i) {
                if (!p1_.tick(p2_, apple_)) {
                    died1 = true;
                    break;
                }
            }
        }

        // P2 tick（即使 P1 死了也要让 P2 完成 tick，不漏双死）
        if (p2_.get_animation_status() == Snake::AnimationStatus::MOVE) {
            for (int i = 0; i < std::max(p2_.get_speed(), 1); ++i) {
                if (!p2_.tick(p1_, apple_)) {
                    died2 = true;
                    break;
                }
            }
        }

        // 统一处理死亡
        auto apply_death = [&](Snake& s) {
            // TIMERACE 下身体不够扣重生成本 → 立即饿死
            if (Global::last_game_mode == Global::GameMode::TIMERACE) {
                int deduct = game_config().reborn_costs;
                if ((int)s.get_body().size() <= deduct) {
                    s.add_score(-deduct);
                    // tick() 记录了碰撞死因（如 ON_WALL），但这里是饿死，覆盖
                    if (s.get_player_id() == 1) Global::player_status1 = Global::PlayerStatus::STARVED;
                    else                        Global::player_status2 = Global::PlayerStatus::STARVED;
                    Global::end_reason = Global::GameOverReason::DEATH;
                    Global::last_score_player1 = p1_.get_score();
                    Global::last_score_player2 = p2_.get_score();
                    finished_ = true;
                    return;
                }
            }
            s.set_animation_status(Snake::AnimationStatus::DYING);
        };
        if (died1) apply_death(p1_);
        if (died2) apply_death(p2_);

        // DEATHMATCH：死的动画，活的立即等待
        if (!finished_ && Global::last_game_mode == Global::GameMode::DEATHMATCH) {
            if (died1 && !died2)
                p2_.set_animation_status(Snake::AnimationStatus::WAITING);
            if (died2 && !died1)
                p1_.set_animation_status(Snake::AnimationStatus::WAITING);
            // died1 && died2 → 都 DYING，在 WAITING 解析处等动画播完
        }
    }

    draw_list_.update();
}

void GameScene::render() {
    // draw grid
    for (int x = 0; x < GRID_W; ++x) {
        for (int y = 0; y < GRID_H; ++y) {
            Rectangle cell{
                (float)(x * CELL_SIZE + OFFSET_X),
                (float)(y * CELL_SIZE + OFFSET_Y),
                (float)CELL_SIZE,
                (float)CELL_SIZE
            };
            DrawRectangleLinesEx(cell, 1, LIGHTGRAY);
        }
    }

    // draw apple
    if (apple_.x >= 0 && apple_.y >= 0) {
        Vector2 ap{
            (float)(apple_.x * CELL_SIZE + OFFSET_X + CELL_SIZE / 2),
            (float)(apple_.y * CELL_SIZE + OFFSET_Y + CELL_SIZE / 2)
        };
        DrawCircleV(ap, CELL_SIZE / 2.0f - 2, RED);
    }

    draw_list_.draw();

    // 统一使用 Snake::get_score() 显示
    DrawText(TextFormat("P1: %d", p1_.get_score()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("P2: %d", p2_.get_score()), 10, 40, 20, DARKBLUE);

    if (Global::last_game_mode == Global::GameMode::TIMERACE) {
        int dur = game_config().time_match_duration;
        if (dur > 0) {
            int remain = std::max(0, dur - (int)time_elapsed_);
            DrawText(TextFormat("TIME: %d", remain), 500, 10, 20, RED);
        } else {
            DrawText("TIME: inf", 500, 10, 20, RED);
        }
    }

    if (pause) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        DrawText("PAUSED", screenW / 2 - MeasureText("PAUSED", 40) / 2, screenH / 2 - 20, 40, GRAY);
        DrawText("Press ESC/BACKSPACE to resume", screenW / 2 - MeasureText("Press ESC/BACKSPACE to resume", 20) / 2, screenH / 2 + 30, 20, GRAY);
        DrawText("Press T to return to menu", screenW / 2 - MeasureText("Press T to return to menu", 20) / 2, screenH / 2 + 60, 20, GRAY);
        DrawText("Press Y to manually end the game", screenW / 2 - MeasureText("Press Y to manually end the game", 20) / 2, screenH / 2 + 90, 20, GRAY);
    }

    DrawFPS(10, 70);
}
