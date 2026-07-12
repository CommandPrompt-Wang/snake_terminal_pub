
#include <raylib.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include "event.h"
#include "snake.h"
#include "config.h"

static bool is_opposite(Event a, Event b) {
    return (a == Event::UP    && b == Event::DOWN)  ||
           (a == Event::DOWN  && b == Event::UP)    ||
           (a == Event::LEFT  && b == Event::RIGHT) ||
           (a == Event::RIGHT && b == Event::LEFT);
}

static Position next_head(const SnakeState &s) {
    Position h = s.body.front();
    switch (s.curDir) {
        case Event::UP:    h.y -= 1; break;
        case Event::DOWN:  h.y += 1; break;
        case Event::LEFT:  h.x -= 1; break;
        case Event::RIGHT: h.x += 1; break;
        default: break;
    }
    return h;
}

static bool on_body(const std::deque<Position> &body, const Position &p, int skipFront = 0) {
    for (size_t i = skipFront; i < body.size(); ++i)
        if (body[i] == p) return true;
    return false;
}

static void init_snake(SnakeState &s, int startX, int startY, Event dir, int len) {
    s.body.clear();
    s.curDir = dir;
    s.curSpeed = 1;
    for (int i = 0; i < len; ++i) {
        if (dir == Event::LEFT)  s.body.push_back({startX + i, startY});
        if (dir == Event::RIGHT) s.body.push_back({startX - i, startY});
        if (dir == Event::UP)    s.body.push_back({startX, startY + i});
        if (dir == Event::DOWN)  s.body.push_back({startX, startY - i});
    }
}

// ── game tick for one player ──────────────────────
static bool tick_player(SnakeState &p, int player, const SnakeState &other,
                        Position &apple) {
    Position head = next_head(p);

    // wall collision
    if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) {
        EventServer::setGameEvent(player, Event::DIE_ON_WALL);
        return false;
    }

    // self collision
    if (on_body(p.body, head, 1)) {
        EventServer::setGameEvent(player, Event::DIE_ON_SELF);
        return false;
    }

    // other-player collision
    if (!game_config().allowThroughTeammates && !other.body.empty() && on_body(other.body, head)) {
        EventServer::setGameEvent(player, Event::DIE_ON_OTHER);
        return false;
    }

    p.body.push_front(head);

    if (head == apple) {
        int score = EventServer::addScore(player, 1);
        EventServer::setScore(player, score);
        apple = random_apple_pos(player == 1 ? p : other, player == 2 ? p : other);
        EventServer::setApplePosition(apple.x, apple.y);
    } else {
        p.body.pop_back();
    }

    return true;
}

void input_thread() {
    SnakeState p1, p2;
    Position apple{-1, -1};
    auto lastTick = std::chrono::steady_clock::now();

    while (!EventServer::is_quit_requested()) {
        PollInputEvents();

        // ── global keys (always active) ──────────
        if (IsKeyPressed(KEY_Q)) {
            EventServer::request_quit();
            break;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            EventServer::setGeneralEvent(Event::PAUSE);
        }

        // ── consume SCENE_CHANGE events (from render/config) ─
        if (EventServer::getEvent(1, EventType::GENERAL) == Event::SCENE_CHANGE) {
            // scene was changed externally, re-read
        }

        // ── scene transitions ─────────────────────
        Scene sc = static_cast<Scene>(EventServer::getScene());

        if (IsKeyPressed(KEY_SPACE)) {
            if (sc == Scene::MENU || sc == Scene::DIE) {
                init_snake(p1, 5, 5, Event::DOWN, 3);
                init_snake(p2, GRID_W - 6, GRID_H - 6, Event::UP, 3);
                apple = random_apple_pos(p1, p2);
                EventServer::setApplePosition(apple.x, apple.y);
                EventServer::setScore(1, 0);
                EventServer::setScore(2, 0);
                EventServer::setSpeedFactor(1, 1);
                EventServer::setSpeedFactor(2, 1);
                EventServer::setGeneralEvent(Event::START);
                EventServer::setScene(static_cast<int>(Scene::GAME));
                EventServer::setGeneralEvent(Event::SCENE_CHANGE);
            }
        }

        // ── direction events (always forwarded) ──
        if (IsKeyPressed(KEY_W))      EventServer::setDirection(1, Event::UP);
        if (IsKeyPressed(KEY_S))      EventServer::setDirection(1, Event::DOWN);
        if (IsKeyPressed(KEY_A))      EventServer::setDirection(1, Event::LEFT);
        if (IsKeyPressed(KEY_D))      EventServer::setDirection(1, Event::RIGHT);
        if (IsKeyPressed(KEY_UP))     EventServer::setDirection(2, Event::UP);
        if (IsKeyPressed(KEY_DOWN))   EventServer::setDirection(2, Event::DOWN);
        if (IsKeyPressed(KEY_LEFT))   EventServer::setDirection(2, Event::LEFT);
        if (IsKeyPressed(KEY_RIGHT))  EventServer::setDirection(2, Event::RIGHT);

        // ── game logic ────────────────────────────
        if (sc == Scene::GAME) {
            auto change_dir = [](SnakeState &s, int player, Event dir) {
                if (is_opposite(s.curDir, dir)) {
                    s.curSpeed = 1;
                    EventServer::setSpeedFactor(player, 1);
                    return;
                }
                s.curDir = dir;
            };

            if (IsKeyPressed(KEY_W))      change_dir(p1, 1, Event::UP);
            if (IsKeyPressed(KEY_S))      change_dir(p1, 1, Event::DOWN);
            if (IsKeyPressed(KEY_A))      change_dir(p1, 1, Event::LEFT);
            if (IsKeyPressed(KEY_D))      change_dir(p1, 1, Event::RIGHT);
            if (IsKeyPressed(KEY_UP))     change_dir(p2, 2, Event::UP);
            if (IsKeyPressed(KEY_DOWN))   change_dir(p2, 2, Event::DOWN);
            if (IsKeyPressed(KEY_LEFT))   change_dir(p2, 2, Event::LEFT);
            if (IsKeyPressed(KEY_RIGHT))  change_dir(p2, 2, Event::RIGHT);

            // speed boost (only if allowed)
            if (game_config().allowAcceleration) {
                int s1 = (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D)) ? 2 : 1;
                int s2 = (IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)) ? 2 : 1;
                p1.curSpeed = s1;
                p2.curSpeed = s2;
                EventServer::setSpeedFactor(1, s1);
                EventServer::setSpeedFactor(2, s2);
            }

            // game tick
            int highScore = std::max(EventServer::getScore(1), EventServer::getScore(2));
            double diffMult = 1.0;
            if (game_config().increasing_difficulty > 0) {
                double k = game_config().increasing_difficulty;
                diffMult = 1.0 + 2.0 * std::log10((k * highScore) / 9.0 + 1.0);
            }
            double tickMs = 500.0 / (diffMult * std::max(p1.curSpeed, p2.curSpeed));
            auto now = std::chrono::steady_clock::now();
            auto interval = std::chrono::milliseconds(static_cast<int>(tickMs));
            if (now - lastTick >= interval) {
                lastTick = now;

                bool alive1 = tick_player(p1, 1, p2, apple);
                bool alive2 = tick_player(p2, 2, p1, apple);

                if (!alive1 || !alive2) {
                    EventServer::setGeneralEvent(Event::STOP);
                    EventServer::setScene(static_cast<int>(Scene::DIE));
                    EventServer::setGeneralEvent(Event::SCENE_CHANGE);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}