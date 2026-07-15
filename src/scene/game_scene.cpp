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

    apple_ = random_apple_pos(p1_, p2_);
    time_elapsed_ = 0;
    time_remaining_ = game_config().time_match_duration;

    tick_remain1_ = 0;
    tick_remain2_ = 0;

    snake_body_1_ = SnakeBody(&p1_, 1);
    snake_body_1_.set_scale({2.0,2.0});
    snake_body_1_.on_die_finished = [&]() {
        if (game_config().respawnInAdvance)
            p1_.generate_ghost();
    };
    draw_list_.push_back(&snake_body_1_);
    snake_body_2_ = SnakeBody(&p2_, 2);
    snake_body_2_.set_scale({2.0,2.0});
    snake_body_2_.on_die_finished = [&]() {
        if (game_config().respawnInAdvance)
            p2_.generate_ghost();
    };
    draw_list_.push_back(&snake_body_2_);

    Global::audio_manager.play_sound("bgm");
}

void GameScene::on_exit() {
    // 停止 BGM
    Global::audio_manager.stop_sound();

    snake_body_1_.print_pos();
    snake_body_2_.print_pos();
    draw_list_.clear();
}

// -- Event handling ---------------------------------
// SceneManager constructs InputEvent and dispatches it here via handle_event()
void GameScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    // 尝试按键重生：WAITING 状态的蛇按下移动键时复活
    auto handle_respawn = [&](Snake& s, const Snake& otherSnake,
                              Direction dir, std::deque<Direction>& pending,
                              SnakeBody& body, SnakeBody& otherBody) -> bool {
        if (!body.is_waiting())
            return false;

        if (game_config().respawnInAdvance) {
            // 虚影模式：部署到预生成的虚影位置（内部处理踩杀）
            // 保存对方部署前的状态，部署后对比判断是否被踩杀
            auto prev_status = (otherSnake.get_player_id() == 1)
                ? Global::player_status1 : Global::player_status2;
            if (!s.deploy_from_ghost(const_cast<Snake&>(otherSnake))) {
                Global::end_reason = Global::GameOverReason::DEATH;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            } else {
                body.switch_to_move();
                pending.push_back(dir);
                // deploy_from_ghost 可能踩杀了对方蛇 → 通过状态变化检测（而非读取可能残留的旧值）
                auto cur_status = (otherSnake.get_player_id() == 1)
                    ? Global::player_status1 : Global::player_status2;
                if (prev_status != Global::PlayerStatus::ON_PLAYER &&
                    cur_status == Global::PlayerStatus::ON_PLAYER) {
                    otherBody.set_dying_interval(last_tick_sec_);
                    Global::audio_manager.play_sfx("die");
                    otherBody.switch_to_die();
                }
            }
        } else {
            if (!s.respawn(otherSnake)) {
                Global::end_reason = Global::GameOverReason::DEATH;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            } else {
                body.switch_to_move();
                pending.push_back(dir);
            }
        }
        return true;
    };

    // 计时赛死亡动画打断辅助
    auto try_interrupt_p1 = [&]() -> bool {
        if (Global::last_game_mode == Global::GameMode::TIMERACE && snake_body_1_.can_interrupt_dying()) {
            snake_body_1_.interrupt_dying();
            return true;
        }
        return false;
    };
    auto try_interrupt_p2 = [&]() -> bool {
        if (Global::last_game_mode == Global::GameMode::TIMERACE && snake_body_2_.can_interrupt_dying()) {
            snake_body_2_.interrupt_dying();
            return true;
        }
        return false;
    };

    switch (event.get_key_code()) {
        // -- P1 移动键（WASD）--
        case KEY_W:
            if (try_interrupt_p1()) break;
            if (handle_respawn(p1_, p2_, Direction::UP, pending_dirs1_, snake_body_1_, snake_body_2_)) break;
            pending_dirs1_.push_back(Direction::UP);
            break;
        case KEY_S:
            if (try_interrupt_p1()) break;
            if (handle_respawn(p1_, p2_, Direction::DOWN, pending_dirs1_, snake_body_1_, snake_body_2_)) break;
            pending_dirs1_.push_back(Direction::DOWN);
            break;
        case KEY_A:
            if (try_interrupt_p1()) break;
            if (handle_respawn(p1_, p2_, Direction::LEFT, pending_dirs1_, snake_body_1_, snake_body_2_)) break;
            pending_dirs1_.push_back(Direction::LEFT);
            break;
        case KEY_D:
            if (try_interrupt_p1()) break;
            if (handle_respawn(p1_, p2_, Direction::RIGHT, pending_dirs1_, snake_body_1_, snake_body_2_)) break;
            pending_dirs1_.push_back(Direction::RIGHT);
            break;
        // -- P2 移动键（IJKL）--
        case KEY_I:
            if (try_interrupt_p2()) break;
            if (handle_respawn(p2_, p1_, Direction::UP, pending_dirs2_, snake_body_2_, snake_body_1_)) break;
            pending_dirs2_.push_back(Direction::UP);
            break;
        case KEY_K:
            if (try_interrupt_p2()) break;
            if (handle_respawn(p2_, p1_, Direction::DOWN, pending_dirs2_, snake_body_2_, snake_body_1_)) break;
            pending_dirs2_.push_back(Direction::DOWN);
            break;
        case KEY_J:
            if (try_interrupt_p2()) break;
            if (handle_respawn(p2_, p1_, Direction::LEFT, pending_dirs2_, snake_body_2_, snake_body_1_)) break;
            pending_dirs2_.push_back(Direction::LEFT);
            break;
        case KEY_L:
            if (try_interrupt_p2()) break;
            if (handle_respawn(p2_, p1_, Direction::RIGHT, pending_dirs2_, snake_body_2_, snake_body_1_)) break;
            pending_dirs2_.push_back(Direction::RIGHT);
            break;
        
        // -- 菜单 --
        case KEY_T:
            if (pause) {
                Global::audio_manager.play_sfx("ui.back");
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

    // 0. 更新计时赛剩余时间并判定超时
    if (Global::last_game_mode == Global::GameMode::TIMERACE) {
        int dur = game_config().time_match_duration;
        if (dur > 0) {
            time_remaining_ = std::max(0, dur - (int)time_elapsed_);
            if (time_remaining_ <= 0) {
                Global::end_reason = Global::GameOverReason::TIMEOUT;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            }
        }
    }

    // 1. 解析 WAITING 状态（由 Snake_Body 渲染层管理 DYING -> WAITING）
    //    - DEATHMATCH: 双蛇 WAITING → 结束（活的蛇已在 tick 后被设为 WAITING）
    //    - TIMERACE:   等待玩家按键重生，这里不做任何操作
    if (Global::last_game_mode == Global::GameMode::DEATHMATCH &&
        snake_body_1_.is_waiting() &&
        snake_body_2_.is_waiting()) {
        Global::end_reason = Global::GameOverReason::DEATH;
        Global::last_score_player1 = p1_.get_score();
        Global::last_score_player2 = p2_.get_score();
        finished_ = true;
    }

    if (finished_) {
        draw_list_.update();
        return;
    }

    // 2. 输入 + 加速
    consume_pending_dir();

    // 3. Tick —— 独立计时器，加速时缩短间隔而非多步子步
    auto calc_tick_ms = [&](int score, int player) {
        double mult = 1.0;
        if (game_config().increasing_difficulty > 0) {
            double k = game_config().increasing_difficulty;
            mult = 1.0 + 2.0 * std::log10((k * std::max(score, 0)) / 9.0 + 1.0);
        }
        float spd = std::max(0.1f, game_config().speed_factor);
        double ms = 500.0 / (mult * spd);
        if (game_config().allowAcceleration) {
            bool shifting = (player == 1) ? IsKeyDown(KEY_LEFT_SHIFT)
                                          : IsKeyDown(KEY_RIGHT_SHIFT);
            if (shifting) ms /= 2.0;
        }
        return (float)(ms / 1000.0);
    };

    // 非 MOVE 状态重置计时器，避免积压；同时预充完整间隔，切回时不突跳
    if (snake_body_1_.is_moving()) tick_remain1_ -= dt;
    else tick_remain1_ = calc_tick_ms(std::max(0, (int)p1_.get_body().size() - 3), 1);
    if (snake_body_2_.is_moving()) tick_remain2_ -= dt;
    else tick_remain2_ = calc_tick_ms(std::max(0, (int)p2_.get_body().size() - 3), 2);

    bool died1 = false, died2 = false;
    using S = Global::PlayerStatus;

    while (tick_remain1_ <= 0 || tick_remain2_ <= 0) {
        bool t1 = tick_remain1_ <= 0 && snake_body_1_.is_moving();
        bool t2 = tick_remain2_ <= 0 && snake_body_2_.is_moving();

        int sc1 = std::max(0, (int)p1_.get_body().size() - 3);
        int sc2 = std::max(0, (int)p2_.get_body().size() - 3);
        float iv1 = calc_tick_ms(sc1, 1);
        float iv2 = calc_tick_ms(sc2, 2);

        if (t1 || t2) {
            auto [r1, r2] = check_collide(p1_, p2_, apple_, t1, t2);
            if (r1 != S::ALIVE) { died1 = true; p1_.set_player_status(r1); }
            if (r2 != S::ALIVE) { died2 = true; p2_.set_player_status(r2); }
            if (r1 == S::ALIVE && t1) { p1_.tick(p2_, apple_); tick_remain1_ += iv1; }
            if (r2 == S::ALIVE && t2) { p2_.tick(p1_, apple_); tick_remain2_ += iv2; }
        } else {
            tick_remain1_ = tick_remain2_ = 0;
            break;
        }
        if (died1 || died2) break;
        last_tick_sec_ = std::min(iv1, iv2);
    }

        // 统一处理死亡
        auto apply_death = [&](Snake& s, SnakeBody& body) {
            if (Global::last_game_mode == Global::GameMode::TIMERACE) {
                int deduct = game_config().reborn_costs;

                if (game_config().respawnInAdvance) {
                    // 虚影模式：死时立即扣分，饿死则直接结束
                    // generate_ghost 由 on_die_finished 回调在死亡动画后触发
                    s.remove_from_back(deduct);
                    s.set_score((int)s.get_body().size() - 3);
                    if (s.get_body().empty()) {
                        if (s.get_player_id() == 1) Global::player_status1 = Global::PlayerStatus::STARVED;
                        else                        Global::player_status2 = Global::PlayerStatus::STARVED;
                        Global::end_reason = Global::GameOverReason::DEATH;
                        Global::last_score_player1 = p1_.get_score();
                        Global::last_score_player2 = p2_.get_score();
                        finished_ = true;
                        return;
                    }
                } else {
                    // 正常模式：身体不够扣 → 立即饿死
                    if ((int)s.get_body().size() <= deduct) {
                        s.add_score(-deduct);
                        if (s.get_player_id() == 1) Global::player_status1 = Global::PlayerStatus::STARVED;
                        else                        Global::player_status2 = Global::PlayerStatus::STARVED;
                        Global::end_reason = Global::GameOverReason::DEATH;
                        Global::last_score_player1 = p1_.get_score();
                        Global::last_score_player2 = p2_.get_score();
                        finished_ = true;
                        return;
                    }
                }
            }
            body.set_dying_interval(last_tick_sec_);
            if (Global::last_game_mode == Global::GameMode::TIMERACE)
                body.set_interrupt_threshold(game_config().deathAnimInterruptThreshold);
            Global::audio_manager.play_sfx("die");
            body.switch_to_die();
        };
        if (died1) apply_death(p1_, snake_body_1_);
        if (died2) apply_death(p2_, snake_body_2_);

        // DEATHMATCH：死的播动画，活的冻结等待
        if (!finished_ && Global::last_game_mode == Global::GameMode::DEATHMATCH) {
            if (died1 && !died2) {
                snake_body_2_.draw_in_waiting = true;
                snake_body_2_.switch_to_waiting();
            }
            if (died2 && !died1) {
                snake_body_1_.draw_in_waiting = true;
                snake_body_1_.switch_to_waiting();
            }
            // died1 && died2 → 都 DYING，在 WAITING 解析处等动画播完
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
            DrawText(TextFormat("TIME: %d", time_remaining_), 500, 10, 20, RED);
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
