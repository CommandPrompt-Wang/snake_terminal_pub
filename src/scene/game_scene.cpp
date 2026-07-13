#include "scene/game_scene.h"
#include "config/config.h"
#include "global.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <iostream>

// -- static helpers --------------------------------

bool GameScene::is_opposite(Direction a, Direction b) {
    return (a == Direction::UP    && b == Direction::DOWN)  ||
           (a == Direction::DOWN  && b == Direction::UP)    ||
           (a == Direction::LEFT  && b == Direction::RIGHT) ||
           (a == Direction::RIGHT && b == Direction::LEFT);
}

Position GameScene::next_head(const SnakeState &s) {
    Position h = s.body.front();
    switch (s.curDir) {
        case Direction::UP:    h.y -= 1; break;
        case Direction::DOWN:  h.y += 1; break;
        case Direction::LEFT:  h.x -= 1; break;
        case Direction::RIGHT: h.x += 1; break;
    }
    return h;
}

bool GameScene::on_body(const std::deque<Position> &body, const Position &p, int skipFront) {
    for (size_t i = skipFront; i < body.size(); ++i)
        if (body[i] == p) return true;
    return false;
}

void GameScene::init_snake(SnakeState &s, int startX, int startY, Direction dir, int len) {
    s.body.clear();
    s.curDir = dir;
    s.lastMoveDir = dir;
    s.curSpeed = 1;
    for (int i = 0; i < len; ++i) {
        if (dir == Direction::LEFT)  s.body.push_back({startX + i, startY});
        if (dir == Direction::RIGHT) s.body.push_back({startX - i, startY});
        if (dir == Direction::UP)    s.body.push_back({startX, startY + i});
        if (dir == Direction::DOWN)  s.body.push_back({startX, startY - i});
    }
}

bool GameScene::tick_player(SnakeState &p, int player, const SnakeState &other, Position &apple) {
    Position head = next_head(p);

    // wall collision / wrap-around
    if (!game_config().toroidalSpace) {
        if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
            if (player == 1) Global::last_game_over_reason = Global::GameOverReason::PLAYER1_ON_WALL;
            else if (player == 2) Global::last_game_over_reason = Global::GameOverReason::PLAYER2_ON_WALL;
            return false;
        }
    } else {
        if (head.x < 0) head.x = GRID_W - 1;
        else if (head.x >= GRID_W) head.x = 0;
        if (head.y < 0) head.y = GRID_H - 1;
        else if (head.y >= GRID_H) head.y = 0;
    }

    // self collision
    if (on_body(p.body, head, 1)) {
        if (player == 1) Global::last_game_over_reason = Global::GameOverReason::PLAYER1_ON_SELF;
        else if (player == 2) Global::last_game_over_reason = Global::GameOverReason::PLAYER2_ON_SELF;
        return false;
    }

    // other-player collision
    if (!game_config().allowThroughOthers && !other.body.empty() && on_body(other.body, head)) {
        if (player == 1) Global::last_game_over_reason = Global::GameOverReason::PLAYER1_ON_PLAYER2;
        else if (player == 2) Global::last_game_over_reason = Global::GameOverReason::PLAYER2_ON_PLAYER1;
        return false;
    }

    p.body.push_front(head);
    p.lastMoveDir = p.curDir;

    if (head == apple) {
        apple = random_apple_pos(/* player1 = */ p, /* player2 = */ other);
    } else {
        p.body.pop_back();
    }

    return true;
}

// -- consume buffered direction into actual snake dir --
void GameScene::consume_pending_dir() {
    while (!pending_dirs1_.empty()) {
        Direction d = pending_dirs1_.front();
        pending_dirs1_.pop_front();
        // 和上一次实际移动方向比较，而非 curDir
        if (!is_opposite(p1_.lastMoveDir, d)) {
            p1_.curDir = d;
        }
    }
    while (!pending_dirs2_.empty()) {
        Direction d = pending_dirs2_.front();
        pending_dirs2_.pop_front();
        if (!is_opposite(p2_.lastMoveDir, d)) {
            p2_.curDir = d;
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

    init_snake(p1_, 5, 5, Direction::DOWN, 3);
    init_snake(p2_, GRID_W - 6, GRID_H - 6, Direction::UP, 3);

    apple_ = random_apple_pos(p1_, p2_);
    score1_ = 0; score2_ = 0;
    speed1_ = 1; speed2_ = 1;

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
        case KEY_UP:    pending_dirs2_.push_back(Direction::UP);    break;
        case KEY_DOWN:  pending_dirs2_.push_back(Direction::DOWN);  break;
        case KEY_LEFT:  pending_dirs2_.push_back(Direction::LEFT);  break;
        case KEY_RIGHT: pending_dirs2_.push_back(Direction::RIGHT); break;
        case KEY_ESCAPE:
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
        case KEY_P:
            pause = !pause;
            event.consume();
            break;
        case KEY_L:
            if (pause) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::DIE);
                Global::last_game_over_reason = Global::GameOverReason::MANUAL;
                event.consume();
            }
    }
}

void GameScene::update(float dt) {
    if (pause) {
        draw_list_.update();
        return;
    }

    consume_pending_dir();
    // speed boost
    if (game_config().allowAcceleration) {
        int s1 = (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) ? 2 : 1;
        int s2 = (IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)) ? 2 : 1;
        p1_.curSpeed = s1;
        p2_.curSpeed = s2;
        speed1_ = s1; speed2_ = s2;
    }

    // tick timing (base rate, unaffected by per-player speed boost)
    int highScore = std::max((int)p1_.body.size() - 3, (int)p2_.body.size() - 3);
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
        for (int i = 0; i < std::max(p1_.curSpeed, 1); ++i) {
            if (alive1) alive1 = tick_player(p1_, 1, p2_, apple_);
        }
        for (int i = 0; i < std::max(p2_.curSpeed, 1); ++i) {
            if (alive2) alive2 = tick_player(p2_, 2, p1_, apple_);
        }

        if (!alive1 || !alive2) {
            Global::last_score_player1 = std::max(0, (int)p1_.body.size() - 3);
            Global::last_score_player2 = std::max(0, (int)p2_.body.size() - 3);
            finished_ = true;
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

    int score1 = std::max(0, (int)p1_.body.size() - 3);
    int score2 = std::max(0, (int)p2_.body.size() - 3);
    DrawText(TextFormat("P1: %d", score1), 10, 10, 20, DARKGREEN);
    DrawText(TextFormat("P2: %d", score2), 10, 40, 20, DARKBLUE);
    
    if (pause) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        DrawText("PAUSED", screenW / 2 - MeasureText("PAUSED", 40) / 2, screenH / 2 - 20, 40, GRAY);
        DrawText("Press P to resume", screenW / 2 - MeasureText("Press P to resume", 20) / 2, screenH / 2 + 30, 20, GRAY);
        DrawText("Press ESC to return to menu", screenW / 2 - MeasureText("Press ESC to return to menu", 20) / 2, screenH / 2 + 60, 20, GRAY);
        DrawText("Press L to manually end the game", screenW / 2 - MeasureText("Press L to manually end the game", 20) / 2, screenH / 2 + 90, 20, GRAY);
    }

    DrawFPS(10, 70);
}
