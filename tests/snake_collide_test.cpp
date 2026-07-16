#include "game/snake.h"
#include "config/config.h"
#include "global.h"

#include <cstdlib>
#include <iostream>
#include <utility>

static int failures = 0;

#define EXPECT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            ++failures; \
        } \
    } while (0)

static Snake make_snake(int id, std::initializer_list<Position> segs, Direction dir) {
    Snake s(id);
    s.init(segs.begin()->x, segs.begin()->y, dir, 1);
    auto& body = s.get_body_mut();
    body.clear();
    for (auto p : segs)
        body.push_back(p);
    s.set_direction(dir);
    s.set_last_move_dir(dir);
    return s;
}

static void reset_config() {
    game_config() = Config{};
}

static void test_wall_collision() {
    reset_config();
    game_config().toroidalSpace = false;

    Snake s1 = make_snake(1, {{0, 5}}, Direction::LEFT);
    Snake s2 = make_snake(2, {{10, 10}}, Direction::UP);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, true, false);
    EXPECT(r1 == Global::PlayerStatus::ON_WALL, "moving left into wall");
    EXPECT(r2 == Global::PlayerStatus::ALIVE, "idle snake stays alive");
}

static void test_toroidal_no_wall() {
    reset_config();
    game_config().toroidalSpace = true;

    Snake s1 = make_snake(1, {{0, 5}}, Direction::LEFT);
    Snake s2 = make_snake(2, {{10, 10}}, Direction::UP);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, true, false);
    EXPECT(r1 == Global::PlayerStatus::ALIVE, "toroidal wrap avoids wall death");
}

static void test_self_collision() {
    reset_config();

    Snake s1 = make_snake(1, {{5, 5}, {5, 6}, {5, 7}}, Direction::DOWN);
    Snake s2 = make_snake(2, {{10, 10}}, Direction::UP);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, true, false);
    EXPECT(r1 == Global::PlayerStatus::ON_SELF, "u-turn into body");
}

static void test_head_to_head() {
    reset_config();

    // 两蛇同时移向同一格 (5,5)，而非相邻格互换（后者不会碰撞）
    Snake s1 = make_snake(1, {{4, 5}}, Direction::RIGHT);
    Snake s2 = make_snake(2, {{6, 5}}, Direction::LEFT);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, true, true);
    EXPECT(r1 == Global::PlayerStatus::ON_PLAYER, "P1 head hits P2 head");
    EXPECT(r2 == Global::PlayerStatus::ON_PLAYER, "P2 head hits P1 head");
}

static void test_adjacent_swap_no_collision() {
    reset_config();

    // 相邻单节对向移动 → 互换格子，同时 tick 时不判撞
    Snake s1 = make_snake(1, {{5, 5}}, Direction::RIGHT);
    Snake s2 = make_snake(2, {{6, 5}}, Direction::LEFT);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, true, true);
    EXPECT(r1 == Global::PlayerStatus::ALIVE, "1-segment swap: P1 survives");
    EXPECT(r2 == Global::PlayerStatus::ALIVE, "1-segment swap: P2 survives");
}

static void test_idle_snake_never_collides() {
    reset_config();

    Snake s1 = make_snake(1, {{5, 5}}, Direction::RIGHT);
    Snake s2 = make_snake(2, {{6, 5}}, Direction::LEFT);
    Position apple{15, 15};

    auto [r1, r2] = check_collide(s1, s2, apple, false, false);
    EXPECT(r1 == Global::PlayerStatus::ALIVE, "non-ticking P1");
    EXPECT(r2 == Global::PlayerStatus::ALIVE, "non-ticking P2");
}

static void test_end_reason_priority() {
    Global::end_reason = Global::GameOverReason::DEATH;
    Global::set_end_reason(Global::GameOverReason::TIMEOUT);
    EXPECT(Global::end_reason == Global::GameOverReason::TIMEOUT, "TIMEOUT beats DEATH");

    Global::end_reason = Global::GameOverReason::TIMEOUT;
    Global::set_end_reason(Global::GameOverReason::DEATH);
    EXPECT(Global::end_reason == Global::GameOverReason::TIMEOUT, "DEATH does not beat TIMEOUT");

    Global::end_reason = Global::GameOverReason::MANUAL;
    Global::set_end_reason(Global::GameOverReason::FULL_BOARD);
    EXPECT(Global::end_reason == Global::GameOverReason::FULL_BOARD, "FULL_BOARD beats MANUAL");
}

int main() {
    test_wall_collision();
    test_toroidal_no_wall();
    test_self_collision();
    test_head_to_head();
    test_adjacent_swap_no_collision();
    test_idle_snake_never_collides();
    test_end_reason_priority();

    if (failures == 0) {
        std::cout << "All snake_tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}
