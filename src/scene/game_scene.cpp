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

    p1_.init(5, 5, Direction::DOWN, 3);
    p2_.init(GRID_W - 6, GRID_H - 6, Direction::UP, 3);
    p1_.set_score(0);
    p2_.set_score(0);

    apple_ = p1_.random_apple_pos(p2_);
    time_elapsed_ = 0;

    last_tick_ = Clock::now();
    
    snake_body_1_ = Snake_Body(&p1_, 1);
    snake_body_1_.set_scale({2.0,2.0});
    draw_list_.push_back(&snake_body_1_);
    snake_body_2_ = Snake_Body(&p2_, 2);
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

    switch (event.get_key_code()) {
        case KEY_W: pending_dirs1_.push_back(Direction::UP);    break;
        case KEY_S: pending_dirs1_.push_back(Direction::DOWN);  break;
        case KEY_A: pending_dirs1_.push_back(Direction::LEFT);  break;
        case KEY_D: pending_dirs1_.push_back(Direction::RIGHT); break;
        case KEY_I:    pending_dirs2_.push_back(Direction::UP);    break;
        case KEY_K:  pending_dirs2_.push_back(Direction::DOWN);  break;
        case KEY_J:  pending_dirs2_.push_back(Direction::LEFT);  break;
        case KEY_L: pending_dirs2_.push_back(Direction::RIGHT); break;
        
        // menu
        case KEY_T:
            if (pause) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::MENU);
            }
            event.consume();
            break;
        // shouldn't do this otherwise will cause segment fault
        // case KEY_Q:
        //     Global::request_quit();
        //     event.consume();
        //     break;
        // default: break;
        case KEY_BACKSPACE:
        case KEY_ESCAPE:
            pause = !pause;
            event.consume();
            break;

        // settling the game
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

    // TIMERACE: 累计经过时间
    time_elapsed_ += dt;

    consume_pending_dir();
    // speed boost
    if (game_config().allowAcceleration) {
        int s1 = IsKeyDown(KEY_LEFT_SHIFT)  ? 2 : 1;
        int s2 = IsKeyDown(KEY_RIGHT_SHIFT) ? 2 : 1;
        p1_.set_speed(s1);
        p2_.set_speed(s2);
    }

    // tick timing (base rate, unaffected by per-player speed boost)
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

        // Each player moves curSpeed times per tick (1 = normal, 2 = boosted)
        bool alive1 = true, alive2 = true;
        for (int i = 0; i < std::max(p1_.get_speed(), 1); ++i) {
            if (alive1) {
                alive1 = p1_.tick(p2_, apple_);
            }
        }
        for (int i = 0; i < std::max(p2_.get_speed(), 1); ++i) {
            if (alive2) {
                alive2 = p2_.tick(p1_, apple_);
            }
        }

        if (Global::last_game_mode == Global::GameMode::TIMERACE) {
            // ── TIMERACE: 死亡复活 + 扣分；仅饿死 / 超时真正结束 ──
            bool starved = false;

            // 复活死亡的玩家（网格满或分数 ≤ -3 时返回 false → 饿死）
            bool s1 = !alive1 && !p1_.respawn(p2_);
            bool s2 = !alive2 && !p2_.respawn(p1_);
            if (s1 || s2) starved = true;

            if (starved) {
                Global::end_reason = Global::GameOverReason::DEATH;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            }

            // 超时检查
            int dur = game_config().time_match_duration;
            if (dur > 0 && time_elapsed_ >= dur) {
                Global::end_reason = Global::GameOverReason::TIMEOUT;
                Global::player_status1 = Global::PlayerStatus::ALIVE;
                Global::player_status2 = Global::PlayerStatus::ALIVE;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            }
        } else {
            // ── DEATHMATCH: 任何死亡都结束 ──
            if (!alive1 || !alive2) {
                Global::end_reason = Global::GameOverReason::DEATH;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            }
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
