#include "scene/game_scene.h"
#include "config/config.h"
#include "global.h"
#include "utility.h"

#include <algorithm>
#include <cmath>

void GameScene::consume_pending_dir() {
    // 每 tick 最多应用一条方向；180° 反向输入被忽略
    while (!pending_dirs1_.empty()) {
        Direction d = pending_dirs1_.front();
        pending_dirs1_.pop_front();
        if (!Snake::is_opposite(p1_.get_last_move_dir(), d))
            p1_.set_direction(d);
    }
    while (!pending_dirs2_.empty()) {
        Direction d = pending_dirs2_.front();
        pending_dirs2_.pop_front();
        if (!Snake::is_opposite(p2_.get_last_move_dir(), d))
            p2_.set_direction(d);
    }
}

void GameScene::sync_scores_and_finish() {
    Global::last_score_player1 = p1_.get_score();
    Global::last_score_player2 = p2_.get_score();
    finished_ = true;
}

void GameScene::setup_snake_body(std::unique_ptr<SnakeBody>& body, Snake& snake, int playerId) {
    body = std::make_unique<SnakeBody>(&snake, playerId);
    body->set_scale({2.0f, 2.0f});
    body->on_die_finished = [this, &snake]() { on_die_finished_handler(snake); };
    draw_list_.push_back(body.get());
}

void GameScene::on_die_finished_handler(Snake& snake) {
    if (Global::last_game_mode != Global::GameMode::TIMERACE)
        return;

    // 扣一条命；命用尽则结束对局
    if (!snake.apply_reborn_cost()) {
        Global::set_end_reason(Global::GameOverReason::DEATH);
        sync_scores_and_finish();
        return;
    }

    if (game_config().respawnInAdvance)
        snake.generate_ghost();  // 预生成幽灵体，复活时直接 deploy
}

GameScene::GameScene()  = default;
GameScene::~GameScene() = default;

void GameScene::on_enter() {
    const char* mode = (Global::last_game_mode == Global::GameMode::DEATHMATCH) ? "DEATHMATCH" : "TIMERACE";
    logd(std::string("game scene enter, mode=") + mode);

    pause = false;
    finished_ = false;
    next_scene_id_ = static_cast<int>(SceneId::DIE);
    pending_dirs1_.clear();
    pending_dirs2_.clear();

    Global::player_status1 = Global::PlayerStatus::ALIVE;
    Global::player_status2 = Global::PlayerStatus::ALIVE;
    Global::end_reason = Global::GameOverReason::NONE;
    Global::last_score_player1 = 0;
    Global::last_score_player2 = 0;

    p1_.init(5, 5, Direction::DOWN, 3);
    p2_.init(GRID_W - 6, GRID_H - 6, Direction::UP, 3);
    p1_.set_score(0);
    p2_.set_score(0);

    apple_ = random_apple_pos(p1_, p2_);
    time_elapsed_ = 0;
    time_remaining_ = game_config().time_match_duration;
    tick_remain1_ = 0;
    tick_remain2_ = 0;

    setup_snake_body(snake_body_1_, p1_, 1);
    setup_snake_body(snake_body_2_, p2_, 2);

    Global::audio_manager.play_sound("bgm");
}

void GameScene::on_exit() {
    logd("game scene exit");
    Global::audio_manager.stop_sound();
    snake_body_1_->print_pos();
    snake_body_2_->print_pos();
    draw_list_.clear();
}

bool GameScene::try_interrupt_dying(SnakeBody& body) {
    if (Global::last_game_mode != Global::GameMode::TIMERACE || !body.can_interrupt_dying())
        return false;
    body.interrupt_dying();
    return true;
}

bool GameScene::handle_respawn(Snake& s, Snake& otherSnake, Direction dir,
                               std::deque<Direction>& pending,
                               SnakeBody& body, SnakeBody& otherBody) {
    if (!body.is_waiting() || Global::last_game_mode != Global::GameMode::TIMERACE)
        return false;

    if (game_config().respawnInAdvance) {
        // 从幽灵体复活；若对方正踩在幽灵格上则触发对方死亡
        auto prev_status = (otherSnake.get_player_id() == 1)
            ? Global::player_status1 : Global::player_status2;
        s.deploy_from_ghost(otherSnake);
        body.switch_to_move();
        pending.push_back(dir);
        auto cur_status = (otherSnake.get_player_id() == 1)
            ? Global::player_status1 : Global::player_status2;
        if (prev_status != Global::PlayerStatus::ON_PLAYER &&
            cur_status == Global::PlayerStatus::ON_PLAYER) {
            otherBody.set_dying_interval(last_tick_sec_);
            Global::audio_manager.play_sfx("die");
            otherBody.switch_to_die();
        }
    } else if (!s.deploy_at_safe_pos(otherSnake)) {
        Global::set_end_reason(Global::GameOverReason::DEATH);
        sync_scores_and_finish();
    } else {
        body.switch_to_move();
        pending.push_back(dir);
    }
    return true;
}

void GameScene::handle_move_key(Snake& s, Snake& otherSnake, Direction dir,
                              std::deque<Direction>& pending,
                              SnakeBody& body, SnakeBody& otherBody) {
    // 优先级：打断死亡动画 > 等待复活 > 普通转向
    if (try_interrupt_dying(body)) return;
    if (handle_respawn(s, otherSnake, dir, pending, body, otherBody)) return;
    pending.push_back(dir);
}

void GameScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_W: handle_move_key(p1_, p2_, Direction::UP, pending_dirs1_, *snake_body_1_, *snake_body_2_); break;
        case KEY_S: handle_move_key(p1_, p2_, Direction::DOWN, pending_dirs1_, *snake_body_1_, *snake_body_2_); break;
        case KEY_A: handle_move_key(p1_, p2_, Direction::LEFT, pending_dirs1_, *snake_body_1_, *snake_body_2_); break;
        case KEY_D: handle_move_key(p1_, p2_, Direction::RIGHT, pending_dirs1_, *snake_body_1_, *snake_body_2_); break;
        case KEY_I: handle_move_key(p2_, p1_, Direction::UP, pending_dirs2_, *snake_body_2_, *snake_body_1_); break;
        case KEY_K: handle_move_key(p2_, p1_, Direction::DOWN, pending_dirs2_, *snake_body_2_, *snake_body_1_); break;
        case KEY_J: handle_move_key(p2_, p1_, Direction::LEFT, pending_dirs2_, *snake_body_2_, *snake_body_1_); break;
        case KEY_L: handle_move_key(p2_, p1_, Direction::RIGHT, pending_dirs2_, *snake_body_2_, *snake_body_1_); break;

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

        case KEY_Y:
            if (pause) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::DIE);
                Global::set_end_reason(Global::GameOverReason::MANUAL);
                Global::player_status1 = Global::PlayerStatus::ALIVE;
                Global::player_status2 = Global::PlayerStatus::ALIVE;
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                event.consume();
            }
            break;
        default:
            break;
    }
}

void GameScene::update(float dt) {
    if (pause) {
        draw_list_.update();  // 暂停时仍更新动画，但不推进 tick
        return;
    }

    time_elapsed_ += dt;

    if (Global::last_game_mode == Global::GameMode::TIMERACE) {
        int dur = game_config().time_match_duration;
        if (dur > 0) {
            time_remaining_ = std::max(0, dur - static_cast<int>(time_elapsed_));
            if (time_remaining_ <= 0) {
                logd("game over: TIMEOUT");
                Global::set_end_reason(Global::GameOverReason::TIMEOUT);
                Global::last_score_player1 = p1_.get_score();
                Global::last_score_player2 = p2_.get_score();
                finished_ = true;
            }
        }
    }

    if (Global::last_game_mode == Global::GameMode::DEATHMATCH &&
        snake_body_1_->is_waiting() &&
        snake_body_2_->is_waiting()) {
        Global::set_end_reason(Global::GameOverReason::DEATH);
        logd("game over: DEATHMATCH both dead");
        Global::last_score_player1 = p1_.get_score();
        Global::last_score_player2 = p2_.get_score();
        finished_ = true;
    }

    if (finished_) {
        draw_list_.update();
        return;
    }

    consume_pending_dir();

    // 根据分数计算 tick 间隔：分数越高越快；Shift 可加速
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
        return static_cast<float>(ms / 1000.0);
    };

    if (snake_body_1_->is_moving()) tick_remain1_ -= dt;
    else tick_remain1_ = calc_tick_ms(std::max(0, static_cast<int>(p1_.get_body().size()) - 3), 1);
    if (snake_body_2_->is_moving()) tick_remain2_ -= dt;
    else tick_remain2_ = calc_tick_ms(std::max(0, static_cast<int>(p2_.get_body().size()) - 3), 2);

    bool died1 = false, died2 = false;
    using S = Global::PlayerStatus;

    // 双蛇 tick 可能同帧触发；循环直到两者均无需 tick 或有人死亡
    while (tick_remain1_ <= 0 || tick_remain2_ <= 0) {
        bool t1 = tick_remain1_ <= 0 && snake_body_1_->is_moving();
        bool t2 = tick_remain2_ <= 0 && snake_body_2_->is_moving();

        int sc1 = std::max(0, static_cast<int>(p1_.get_body().size()) - 3);
        int sc2 = std::max(0, static_cast<int>(p2_.get_body().size()) - 3);
        float iv1 = calc_tick_ms(sc1, 1);
        float iv2 = calc_tick_ms(sc2, 2);

        if (t1 || t2) {
            auto [r1, r2] = check_collide(p1_, p2_, apple_, t1, t2);
            if (r1 != S::ALIVE || r2 != S::ALIVE)
                logd("collision: P1=" + std::to_string(static_cast<int>(r1)) + " P2=" + std::to_string(static_cast<int>(r2)));
            if (r1 != S::ALIVE) { died1 = true; p1_.set_player_status(r1); }
            if (r2 != S::ALIVE) { died2 = true; p2_.set_player_status(r2); }
            if (r1 == S::ALIVE && t1) {
                auto tr1 = p1_.tick(p2_, apple_);
                tick_remain1_ += iv1;
                if (tr1 != S::ALIVE) died1 = true;
            }
            if (r2 == S::ALIVE && t2) {
                auto tr2 = p2_.tick(p1_, apple_);
                tick_remain2_ += iv2;
                if (tr2 != S::ALIVE) died2 = true;
            }
        } else {
            tick_remain1_ = tick_remain2_ = 0;
            break;
        }
        if (died1 || died2) {
            last_tick_sec_ = std::min(iv1, iv2);
            break;
        }
        last_tick_sec_ = std::min(iv1, iv2);
    }

    auto apply_death = [&](Snake& s, SnakeBody& body) {
        auto status = (s.get_player_id() == 1) ? Global::player_status1 : Global::player_status2;
        logd("P" + std::to_string(s.get_player_id()) + " died, status=" + std::to_string(static_cast<int>(status)));
        body.set_dying_interval(last_tick_sec_);
        if (Global::last_game_mode == Global::GameMode::TIMERACE)
            body.set_interrupt_threshold(game_config().deathAnimInterruptThreshold);
        Global::audio_manager.play_sfx("die");
        body.switch_to_die();
    };
    if (died1) apply_death(p1_, *snake_body_1_);
    if (died2) apply_death(p2_, *snake_body_2_);

    // DEATHMATCH：一方死亡后另一方进入 waiting，等待其也死亡才结束
    if (!finished_ && Global::last_game_mode == Global::GameMode::DEATHMATCH) {
        if (died1 && !died2) {
            snake_body_2_->draw_in_waiting = true;
            snake_body_2_->switch_to_waiting();
        }
        if (died2 && !died1) {
            snake_body_1_->draw_in_waiting = true;
            snake_body_1_->switch_to_waiting();
        }
    }

    draw_list_.update();
}

void GameScene::render() {
    // 网格线
    for (int x = 0; x < GRID_W; ++x) {
        for (int y = 0; y < GRID_H; ++y) {
            Rectangle cell{
                grid_to_pixel_x(x),
                grid_to_pixel_y(y),
                static_cast<float>(CELL_SIZE),
                static_cast<float>(CELL_SIZE)
            };
            DrawRectangleLinesEx(cell, 1, LIGHTGRAY);
        }
    }

    if (apple_.x >= 0 && apple_.y >= 0) {
        apple.set_pos({grid_to_pixel_x(apple_.x), grid_to_pixel_y(apple_.y)});
        apple.set_scale({2.0f, 2.0f});
        apple.draw();  // 直接 draw，不加入 draw_list_
    }

    draw_list_.draw();  // 蛇身 + DrawLayer flush

    DrawText(TextFormat("P1: %d", p1_.get_score()), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("P2: %d", p2_.get_score()), 10, 40, 20, DARKBLUE);

    if (Global::last_game_mode == Global::GameMode::TIMERACE) {
        int dur = game_config().time_match_duration;
        if (dur > 0)
            DrawText(TextFormat("TIME: %d", time_remaining_), 500, 10, 20, RED);
        else
            DrawText("TIME: inf", 500, 10, 20, RED);
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
