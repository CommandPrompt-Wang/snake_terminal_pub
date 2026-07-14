#pragma once
#include <deque>
#include <random>
#include "utility.h"
#include "config/config.h"
#include "global.h"

// ── Snake 类 ──────────────────────────────────────
// 封装蛇的全部状态和游戏逻辑（移动、碰撞、复活、苹果生成等）
class Snake {
public:
    Snake(int playerId = 0);

    // ── 访问器 ──
    const std::deque<Position>& get_body() const { return body_; }
    std::deque<Position>&       get_body_mut()   { return body_; }
    Direction  get_direction()     const { return curDir_; }
    Direction  get_last_move_dir() const { return lastMoveDir_; }
    int        get_speed()         const { return curSpeed_; }
    int        get_player_id()     const { return playerId_; }
    int        get_score()         const { return score_; }
    size_t     get_length()        const { return body_.size(); }

    // ── 修改器 ──
    void set_direction(Direction d)     { curDir_ = d; }
    void set_last_move_dir(Direction d) { lastMoveDir_ = d; }
    void set_speed(int s)               { curSpeed_ = s; }
    void set_score(int s)               { score_ = s; }
    void add_score(int delta)           { score_ += delta; }

    // ── 初始化 / 重置 ──
    void init(int startX, int startY, Direction dir, int len);
    void reset();

    // ── 方向工具 ──
    static bool is_opposite(Direction a, Direction b);

    // ── 碰撞检测 ──
    Position next_head() const;
    bool check_self_collision(const Position& h, int skipFront = 1) const;
    bool check_body_collision(const std::deque<Position>& body, const Position& h) const;

    // ── 移动 ──
    void push_head(const Position& h);
    void pop_tail();

    // ── 高层操作 ──
    bool tick(const Snake& other, Position& apple);       // 单次移动 tick
    bool respawn(const Snake& other);                     // 复活（扣除分 -> 重定位）
    void remove_from_back(int count);                     // 从尾部切除
    void translate(int dx, int dy);                       // 整条蛇平移

    // ── 苹果 / 安全位置 ──
    Position random_apple_pos(const Snake& other) const;
    Position random_safe_pos(const Snake& other) const;

private:
    void set_player_status(Global::PlayerStatus status) const;

    std::deque<Position> body_;
    Direction curDir_ = Direction::DOWN;
    Direction lastMoveDir_ = Direction::DOWN;
    int curSpeed_ = 1;
    int playerId_ = 0;
    int score_ = 0;

    static std::mt19937 rng_;
};
