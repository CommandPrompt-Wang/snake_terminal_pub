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
    s.curSpeed = 1;
    for (int i = 0; i < len; ++i) {
        if (dir == Direction::LEFT)  s.body.push_back({startX + i, startY});
        if (dir == Direction::RIGHT) s.body.push_back({startX - i, startY});
        if (dir == Direction::UP)    s.body.push_back({startX, startY + i});
        if (dir == Direction::DOWN)  s.body.push_back({startX, startY - i});
    }
}

bool GameScene::tick_player(SnakeState &p, int /*player*/, const SnakeState &other, Position &apple) {
    Position head = next_head(p);

    // wall collision
    if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
        return false;
    }

    // self collision
    if (on_body(p.body, head, 1)) {
        return false;
    }

    // other-player collision
    if (!game_config().allowThroughTeammates && !other.body.empty() && on_body(other.body, head)) {
        return false;
    }

    p.body.push_front(head);

    if (head == apple) {
        apple = random_apple_pos(/* player1 = */ p, /* player2 = */ other);
    } else {
        p.body.pop_back();
    }

    return true;
}

// -- consume buffered direction into actual snake dir --
void GameScene::consume_pending_dir() {
    if (has_pending_dir1_ && !is_opposite(p1_.curDir, pending_dir1_)) {
        p1_.curDir = pending_dir1_;
    }
    if (has_pending_dir2_ && !is_opposite(p2_.curDir, pending_dir2_)) {
        p2_.curDir = pending_dir2_;
    }
    has_pending_dir1_ = false;
    has_pending_dir2_ = false;
}

// -- constructor / destructor ---------------------

GameScene::GameScene()  = default;
GameScene::~GameScene() = default;

Snake_Body snake_body_1,snake_body_2;

void GameScene::on_enter() {
    finished_ = false;
    next_scene_id_ = static_cast<int>(SceneId::DIE);
    has_pending_dir1_ = false;
    has_pending_dir2_ = false;

    init_snake(p1_, 5, 5, Direction::DOWN, 3);
    init_snake(p2_, GRID_W - 6, GRID_H - 6, Direction::UP, 3);

    apple_ = random_apple_pos(p1_, p2_);
    score1_ = 0; score2_ = 0;
    speed1_ = 1; speed2_ = 1;

    last_tick_ = Clock::now();
    
    snake_body_1 = Snake_Body(&p1_, 1);
    snake_body_1.set_scale({2.0,2.0});
    draw_list_.push_back(&snake_body_1);
    snake_body_2 = Snake_Body(&p2_, 2);
    snake_body_2.set_scale({2.0,2.0});
    draw_list_.push_back(&snake_body_2);
}

void GameScene::on_exit() {
    snake_body_1.print_pos();
    snake_body_2.print_pos();
    draw_list_.clear();
}

// -- Event handling ---------------------------------
// SceneManager constructs InputEvent and dispatches it here via handle_event()
void GameScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_W: pending_dir1_ = Direction::UP;    has_pending_dir1_ = true; break;
        case KEY_S: pending_dir1_ = Direction::DOWN;  has_pending_dir1_ = true; break;
        case KEY_A: pending_dir1_ = Direction::LEFT;  has_pending_dir1_ = true; break;
        case KEY_D: pending_dir1_ = Direction::RIGHT; has_pending_dir1_ = true; break;
        case KEY_UP:    pending_dir2_ = Direction::UP;    has_pending_dir2_ = true; break;
        case KEY_DOWN:  pending_dir2_ = Direction::DOWN;  has_pending_dir2_ = true; break;
        case KEY_LEFT:  pending_dir2_ = Direction::LEFT;  has_pending_dir2_ = true; break;
        case KEY_RIGHT: pending_dir2_ = Direction::RIGHT; has_pending_dir2_ = true; break;
        case KEY_ESCAPE:
            finished_ = true;
            next_scene_id_ = static_cast<int>(SceneId::MENU);
            event.consume();
            break;
        // shouldn't do this otherwise will cause segment fault
        // case KEY_Q:
        //     Global::request_quit();
        //     event.consume();
        //     break;
        // default: break;
    }
}

void GameScene::update(float dt) {
    consume_pending_dir();
    // speed boost
    if (game_config().allowAcceleration) {
        int s1 = (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) ? 2 : 1;
        int s2 = (IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)) ? 2 : 1;
        p1_.curSpeed = s1;
        p2_.curSpeed = s2;
        speed1_ = s1; speed2_ = s2;
    }

    // tick timing
    int highScore = std::max((int)p1_.body.size() - 3, (int)p2_.body.size() - 3);
    double diffMult = 1.0;
    if (game_config().increasing_difficulty > 0) {
        double k = game_config().increasing_difficulty;
        diffMult = 1.0 + 2.0 * std::log10((k * highScore) / 9.0 + 1.0);
    }
    double tickMs = 500.0 / (diffMult * std::max(p1_.curSpeed, p2_.curSpeed));
    auto now = Clock::now();
    auto interval = std::chrono::milliseconds(static_cast<int>(tickMs));
    if (now - last_tick_ >= interval) {
        last_tick_ = now;

        bool alive1 = tick_player(p1_, 1, p2_, apple_);
        bool alive2 = tick_player(p2_, 2, p1_, apple_);

        // apple position already in apple_ member

        if (!alive1 || !alive2) {
            finished_ = true;
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
    DrawFPS(10, 70);
}
