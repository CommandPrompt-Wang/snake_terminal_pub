#include "game/snake.h"
#include "global.h"
#include "utils/log.h"

// -- 静态成员 --
std::mt19937 Snake::rng_{std::random_device{}()};

// -- 构造 --
Snake::Snake(int playerId)
    : playerId_(playerId) {}

// -- 初始化 / 重置 --

void Snake::init(int startX, int startY, Direction dir, int len) {
    body_.clear();
    curDir_ = dir;
    lastMoveDir_ = dir;
    is_ghost_ = false;
    collidable_ = true;
    for (int i = 0; i < len; ++i) {
        if (dir == Direction::LEFT)  body_.push_back({startX + i, startY});
        if (dir == Direction::RIGHT) body_.push_back({startX - i, startY});
        if (dir == Direction::UP)    body_.push_back({startX, startY + i});
        if (dir == Direction::DOWN)  body_.push_back({startX, startY - i});
    }
}

void Snake::reset() {
    body_.clear();
    curDir_ = Direction::DOWN;
    lastMoveDir_ = Direction::DOWN;
    score_ = 0;
    is_ghost_ = false;
    collidable_ = true;
}

// -- 方向工具 --

bool Snake::is_opposite(Direction a, Direction b) {
    return (a == Direction::UP    && b == Direction::DOWN)  ||
           (a == Direction::DOWN  && b == Direction::UP)    ||
           (a == Direction::LEFT  && b == Direction::RIGHT) ||
           (a == Direction::RIGHT && b == Direction::LEFT);
}

// -- 碰撞检测 --

Position Snake::next_head() const {
    Position h = body_.front();
    switch (curDir_) {
        case Direction::UP:    h.y -= 1; break;
        case Direction::DOWN:  h.y += 1; break;
        case Direction::LEFT:  h.x -= 1; break;
        case Direction::RIGHT: h.x += 1; break;
    }
    return h;
}

bool Snake::check_self_collision(const Position& h, int skipFront) const {
    for (size_t i = skipFront; i < body_.size(); ++i)
        if (body_[i] == h) return true;
    return false;
}

bool Snake::check_body_collision(const std::deque<Position>& body, const Position& h) const {
    for (auto& seg : body)
        if (seg == h) return true;
    return false;
}

// -- 移动 --

void Snake::push_head(const Position& h) {
    body_.push_front(h);
    lastMoveDir_ = curDir_;
}

void Snake::pop_tail() {
    body_.pop_back();
}

// -- 高层操作 --

/// 碰撞预判。tick=false 的蛇不移动，其碰撞结果恒为 ALIVE
std::pair<Global::PlayerStatus, Global::PlayerStatus>
check_collide(const Snake& s1, const Snake& s2, const Position& apple,
              bool tick1, bool tick2) {
    using S = Global::PlayerStatus;
    auto A = S::ALIVE;

    auto wrap = [](Position h) {
        if (!game_config().toroidalSpace) return h;
        if (h.x < 0) h.x = GRID_W - 1; else if (h.x >= GRID_W) h.x = 0;
        if (h.y < 0) h.y = GRID_H - 1; else if (h.y >= GRID_H) h.y = 0;
        return h;
    };

    Position h1 = wrap(s1.next_head());
    Position h2 = wrap(s2.next_head());

    auto wall  = [&](Position h) { return !game_config().toroidalSpace &&
        (h.x < 0 || h.x >= GRID_W || h.y < 0 || h.y >= GRID_H); };
    auto self  = [&](const Snake& s, Position h) {
        int skip = (h == apple) ? 0 : 1;
        for (size_t i = skip; i < s.body_.size(); ++i)
            if (s.body_[i] == h) return true;
        return false;
    };

    S r1 = A, r2 = A;

    if (tick1 && wall(h1))  r1 = S::ON_WALL;
    if (tick2 && wall(h2))  r2 = S::ON_WALL;
    if (r1 != A || r2 != A) return {r1, r2};

    if (tick1 && self(s1, h1)) r1 = S::ON_SELF;
    if (tick2 && self(s2, h2)) r2 = S::ON_SELF;
    if (r1 != A || r2 != A) return {r1, r2};

    if (!game_config().allowThroughOthers && s1.collidable_ && s2.collidable_) {
        // 使用原 deque 引用进行分段检查，避免每帧深拷贝
        // 判断 h1 是否撞入 s2 的"模拟后身体"（排除 s2 的旧头部 index=0）
        auto hits = [&](const std::deque<Position>& targetBody, Position myHead,
                        Position otherNewHead, bool otherEatsApple, bool otherTicking) {
            if (!otherTicking) {
                // 对方不动：检测完整当前身体（含头部 index=0 和尾部）
                for (size_t i = 0; i < targetBody.size(); ++i)
                    if (targetBody[i] == myHead) return true;
                return false;
            }
            // 对方移动：模拟 push_front(otherNewHead)，若未吃苹果则 pop_back
            size_t end = otherEatsApple ? targetBody.size() : targetBody.size() - 1;
            for (size_t i = 1; i < end; ++i)
                if (targetBody[i] == myHead) return true;
            if (myHead == otherNewHead) return true;
            return false;
        };
        bool hit12 = tick1 && hits(s2.body_, h1, h2, h2 == apple, tick2);
        bool hit21 = tick2 && hits(s1.body_, h2, h1, h1 == apple, tick1);
        if (hit12) r1 = S::ON_PLAYER;
        if (hit21) r2 = S::ON_PLAYER;
    }

    return {r1, r2};
}

Global::PlayerStatus Snake::tick(const Snake& other, Position& apple) {
    Position head = next_head();

    // 环形卷绕
    if (game_config().toroidalSpace) {
        if (head.x < 0) head.x = GRID_W - 1;
        else if (head.x >= GRID_W) head.x = 0;
        if (head.y < 0) head.y = GRID_H - 1;
        else if (head.y >= GRID_H) head.y = 0;
    }

    body_.push_front(head);
    lastMoveDir_ = curDir_;

    if (head == apple) {
        ++score_;
        logd("P" + std::to_string(playerId_) + " eat apple, score=" + std::to_string(score_));
        Global::audio_manager.play_sfx("eat");
        apple = random_apple_pos(*this, other);
        if (apple.x < 0) {
            logw("board full, no apple position");
            Global::set_end_reason(Global::GameOverReason::FULL_BOARD);
            set_player_status(Global::PlayerStatus::STARVED);
            return Global::PlayerStatus::STARVED;
        }
    } else {
        body_.pop_back();
    }

    return Global::PlayerStatus::ALIVE;
}

bool Snake::apply_reborn_cost() {
    int deduct = game_config().reborn_costs;
    remove_from_back(deduct);
    if (body_.empty()) {
        add_score(-deduct);
        logw("P" + std::to_string(playerId_) + " starved: body empty after reborn cost");
        set_player_status(Global::PlayerStatus::STARVED);
        return false;
    }
    set_score(static_cast<int>(body_.size()) - 3);
    return true;
}

bool Snake::deploy_at_safe_pos(const Snake& other) {
    if (body_.empty()) {
        set_player_status(Global::PlayerStatus::STARVED);
        return false;
    }

    Position pos = random_safe_pos(*this, other);
    if (pos.x < 0 || pos.y < 0) {
        logw("P" + std::to_string(playerId_) + " respawn failed: no safe position");
        set_player_status(Global::PlayerStatus::STARVED);
        return false;
    }

    int dx = pos.x - body_[0].x;
    int dy = pos.y - body_[0].y;
    translate(dx, dy);
    set_player_status(Global::PlayerStatus::ALIVE);
    return true;
}

void Snake::remove_from_back(int count) {
    int to_remove = std::min(count, (int)body_.size());
    for (int i = 0; i < to_remove; ++i)
        body_.pop_back();
}

void Snake::translate(int dx, int dy) {
    for (auto& seg : body_) {
        seg.x += dx;
        seg.y += dy;
    }
}
// ── 虚影 ──

void Snake::set_ghost(bool on) {
    is_ghost_   = on;
    collidable_ = !on;
}

void Snake::generate_ghost() {
    std::uniform_int_distribution<int> dx(0, GRID_W - 1);
    std::uniform_int_distribution<int> dy(0, GRID_H - 1);
    Position target = {dx(rng_), dy(rng_)};

    logd("[ghost] P" + std::to_string(playerId_) + " ghost at (" + std::to_string(target.x) + "," + std::to_string(target.y)
         + ") from (" + std::to_string(body_[0].x) + "," + std::to_string(body_[0].y) + ")");

    int ddx = target.x - body_[0].x;
    int ddy = target.y - body_[0].y;
    for (auto& seg : body_) { seg.x += ddx; seg.y += ddy; }

    set_ghost(true);
}

void Snake::deploy_from_ghost(Snake& other) {
    logd("[ghost] P" + std::to_string(playerId_) + " deploy at (" + std::to_string(body_[0].x) + "," + std::to_string(body_[0].y) + ")");

    set_ghost(false);
    set_player_status(Global::PlayerStatus::ALIVE);

    if (check_body_collision(other.body_, body_.front())) {
        other.set_player_status(Global::PlayerStatus::ON_PLAYER);
    }
}
// -- 苹果 / 安全位置（friend 自由函数）--

Position random_apple_pos(const Snake& a, const Snake& b) {
    std::uniform_int_distribution<int> dx(0, GRID_W - 1);
    std::uniform_int_distribution<int> dy(0, GRID_H - 1);

    for (int tries = 0; tries < 200; ++tries) {
        Position p{dx(Snake::rng_), dy(Snake::rng_)};
        bool on_snake = false;
        for (auto& seg : a.body_) if (seg == p) { on_snake = true; break; }
        if (!on_snake)
            for (auto& seg : b.body_) if (seg == p) { on_snake = true; break; }
        if (!on_snake) return p;
    }
    return {-1, -1}; // grid full
}

Position random_safe_pos(const Snake& a, const Snake& b) {
    std::uniform_int_distribution<int> dx(0, GRID_W - 1);
    std::uniform_int_distribution<int> dy(0, GRID_H - 1);

    for (int tries = 0; tries < 200; ++tries) {
        Position p{dx(Snake::rng_), dy(Snake::rng_)};
        bool on_snake = false;
        for (auto& seg : a.body_) if (seg == p) { on_snake = true; break; }
        if (!on_snake)
            for (auto& seg : b.body_) if (seg == p) { on_snake = true; break; }
        if (!on_snake) return p;
    }
    return {-1, -1}; // grid full
}

// -- 内部工具 --

void Snake::set_player_status(Global::PlayerStatus status) const {
    if (playerId_ == 1)      Global::player_status1 = status;
    else if (playerId_ == 2) Global::player_status2 = status;
}
