#pragma once
#include "game/snake_render.h"
#include "game/snake_state/animate_manager.h"
#include "game/snake.h"
#include "config/config.h"
#include "render/sprite.h"
#include "raylib.h"
#include <vector>
#include <functional>

class SnakeBody;

// ============================================================
// SnakeMove — 蛇移动/加速动画状态
// ============================================================
class SnakeMove : public AnimateState
{
    friend class SnakeBody;
private:
    SnakeBody* snakebody;
    float frame_process = 0;
    int speedup_offset = 0;
    static constexpr float speedup_duration = 0.6f;
    static constexpr int speedup_loop_length = 4, speedup_length = 2;
public:
    SnakeMove(SnakeBody* sb) : snakebody(sb) {}
    void update() override;
    void on_enter() override;
    void draw() override;
    void on_exit() override {}
};

// ============================================================
// SnakeDie — 蛇死亡动画状态
// ============================================================
class SnakeDie : public AnimateState
{
    friend class SnakeBody;
private:
    SnakeBody* snakebody;
    bool pause = false;
    float dying_process = 0;
    int dying_offset = 0;
    std::vector<Sprite> die_block;
    float dying_duration = 0.3f;  // 每节 X 间隔（秒），由游戏 tick 速率同步
    static constexpr int dying_length = 1;
public:
    SnakeDie(SnakeBody* sb) : snakebody(sb) {}
    void on_enter() override;
    void update() override;
    void draw() override;
    void on_exit() override {}

    // 打断死亡动画：跳过未播放的 X 标记
    bool can_interrupt() const;
    void interrupt();
    int interrupt_threshold = 0;
};

// ============================================================
// SnakeWaiting — 等待复活状态
// ============================================================
class SnakeWaiting : public AnimateState
{
    friend class SnakeBody;
private:
    SnakeBody* snakebody;
public:
    SnakeWaiting(SnakeBody* sb) : snakebody(sb) {}
    void on_enter() override { next_state = ""; }
    void update() override {}
    void draw() override;
    void on_exit() override {}
};

// ============================================================
// SnakeBody — 蛇身渲染体
// ============================================================
class SnakeBody : public BasicRenderClass
{
protected:
    Sprite head[2] = {Sprite("resources/img/up_head.png",10),Sprite("resources/img/right_head.png",10)};
    std::vector<SnakeBlock> body;
    Snake* snake;
    int playerid;
    bool is_hide = 0;
    Vector2 scale = {1, 1};

    AnimateManager animate_manager;
    SnakeMove snakemove;
    SnakeDie snakedie;
    SnakeWaiting snakewaiting;

    friend class SnakeMove;
    friend class SnakeDie;
    friend class SnakeWaiting;

public:
    SnakeBody(Snake* snake = nullptr, int playerid = 0)
        : snake(snake), playerid(playerid), snakemove(this), snakedie(this), snakewaiting(this)
    {
        animate_manager.add_state("snake_move", snakemove);
        animate_manager.add_state("snake_die", snakedie);
        animate_manager.add_state("snake_waiting", snakewaiting);
        animate_manager.switch_state("snake_move");
    }

    SnakeBody(SnakeBody&& other) noexcept
        : head{std::move(other.head[0]), std::move(other.head[1])}
        , body(std::move(other.body))
        , snake(other.snake)
        , playerid(other.playerid)
        , is_hide(other.is_hide)
        , scale(other.scale)
        , animate_manager(std::move(other.animate_manager))
        , snakemove(std::move(other.snakemove))
        , snakedie(std::move(other.snakedie))
        , snakewaiting(std::move(other.snakewaiting))
        , on_die_finished(std::move(other.on_die_finished))
        , draw_in_waiting(other.draw_in_waiting)
    {
        snakemove.snakebody = this;
        snakedie.snakebody = this;
        snakewaiting.snakebody = this;
        animate_manager.add_state("snake_move", snakemove);
        animate_manager.add_state("snake_die", snakedie);
        animate_manager.add_state("snake_waiting", snakewaiting);
    }
    SnakeBody& operator=(SnakeBody&& other) noexcept {
        if (this == &other) return *this;
        head[0] = std::move(other.head[0]);
        head[1] = std::move(other.head[1]);
        body = std::move(other.body);
        snake = other.snake;
        playerid = other.playerid;
        is_hide = other.is_hide;
        scale = other.scale;
        animate_manager = std::move(other.animate_manager);
        snakemove = std::move(other.snakemove);
        snakedie = std::move(other.snakedie);
        snakewaiting = std::move(other.snakewaiting);
        on_die_finished = std::move(other.on_die_finished);
        draw_in_waiting = other.draw_in_waiting;
        snakemove.snakebody = this;
        snakedie.snakebody = this;
        snakewaiting.snakebody = this;
        animate_manager.add_state("snake_move", snakemove);
        animate_manager.add_state("snake_die", snakedie);
        animate_manager.add_state("snake_waiting", snakewaiting);
        return *this;
    }
    SnakeBody(const SnakeBody&) = delete;
    SnakeBody& operator=(const SnakeBody&) = delete;

    void set_scale(Vector2 s) {
        scale = s;
        for (auto& i : body) i.set_scale(s);
        head[0].set_scale(s);
        head[1].set_scale(s);
    }
    void set_hide(bool h) { is_hide = h; }

    bool is_moving()  const { return animate_manager.get_current_state() == "snake_move"; }
    bool is_dying()   const { return animate_manager.get_current_state() == "snake_die"; }
    bool is_waiting() const { return animate_manager.get_current_state() == "snake_waiting"; }

    void switch_to_move()    { draw_in_waiting = false; animate_manager.switch_state("snake_move"); }
    void switch_to_die()     { animate_manager.switch_state("snake_die"); }
    void set_dying_interval(float sec) { snakedie.dying_duration = sec; }
    void set_interrupt_threshold(int t) { snakedie.interrupt_threshold = t; }
    bool can_interrupt_dying() const { return is_dying() && snakedie.can_interrupt(); }
    void interrupt_dying() { snakedie.interrupt(); }
    void switch_to_waiting() { animate_manager.switch_state("snake_waiting"); }

    // 死亡动画播放完毕后，由 update() 自动处理虚影部署
    std::function<void()> on_die_finished;

    // DEATHMATCH 幸存者进入 WAITING 时需保持可见
    bool draw_in_waiting = false;

    void update() {
        if (snake == nullptr) return;
        std::string prev = animate_manager.get_current_state();

        animate_manager.update();

        // 死亡动画完成 → 执行回调（部署虚影等），然后刷新渲染状态
        if (prev == "snake_die" && animate_manager.get_current_state() == "snake_waiting") {
            if (on_die_finished) on_die_finished();
            _apply_ghost_alpha();
            _sync_body_from_snake();
        } else {
            _apply_ghost_alpha();
        }
    }

private:
    void _apply_ghost_alpha() {
        uint8_t alpha = snake->is_ghost() ? 100 : 255;
        for (auto& b : body) b.set_alpha(alpha);
        head[0].set_alpha(alpha);
        head[1].set_alpha(alpha);
    }

    void _sync_body_from_snake() {
        const auto& logicalBody = snake->get_body();
        while (body.size() < logicalBody.size()) {
            body.emplace_back(playerid, (Vector2){0, 0});
            body.back().set_scale(scale);
        }
        while (body.size() > logicalBody.size()) {
            body.pop_back();
        }
        for (size_t i = 0; i < body.size(); i++) {
            body[i].set_pos(logicalBody[i].x, logicalBody[i].y);
        }
        if (!logicalBody.empty()) {
            head[0].set_pos(logicalBody[0].x * 32, logicalBody[0].y * 32 + 200);
            head[1].set_pos(logicalBody[0].x * 32, logicalBody[0].y * 32 + 200);
        }
    }

public:
    void draw() {
        if (snake == nullptr || is_hide) return;
        animate_manager.draw();
    }
    void print_pos() {
        std::cerr << "snake : " << playerid << '\n';
        std::cerr << "head0 = " << head[0].get_hide() << " head1 = " << head[1].get_hide() << '\n';
        for (auto& i : body) {
            std::cerr << "##############(" << i.get_pos().x << ", " << i.get_pos().y << ")\n";
            std::cerr << "###############" << i.get_status() << '\n';
        }
    }
    ~SnakeBody() {}
};

// ============================================================
// 动画状态方法实现
// ============================================================

inline void SnakeMove::on_enter() {
    speedup_offset = 0;
    frame_process = 0;
    next_state = "";
}

inline void SnakeMove::update() {
    switch (snakebody->snake->get_direction()) {
        case Direction::UP:
            snakebody->head[0].set_flip_v(0); snakebody->head[0].set_hide(0);
            snakebody->head[1].set_hide(1);
            break;
        case Direction::RIGHT:
            snakebody->head[1].set_flip_h(0); snakebody->head[1].set_hide(0);
            snakebody->head[0].set_hide(1);
            break;
        case Direction::DOWN:
            snakebody->head[0].set_flip_v(1); snakebody->head[0].set_hide(0);
            snakebody->head[1].set_hide(1);
            break;
        case Direction::LEFT:
            snakebody->head[1].set_flip_h(1); snakebody->head[1].set_hide(0);
            snakebody->head[0].set_hide(1);
            break;
    }
    snakebody->head[0].update(); snakebody->head[1].update();
    bool is_speedup = game_config().allowAcceleration && (
        (snakebody->playerid == 1)
            ? IsKeyDown(KEY_LEFT_SHIFT)
            : IsKeyDown(KEY_RIGHT_SHIFT));
    if (is_speedup) {
        frame_process += GetFrameTime();
        while (frame_process > speedup_duration) {
            frame_process -= speedup_duration;
            speedup_offset = (speedup_offset + 1) % speedup_loop_length;
        }
    } else {
        frame_process = 0;
        speedup_offset = 0;
    }
    while (snakebody->body.size() < snakebody->snake->get_body().size()) {
        snakebody->body.emplace_back(snakebody->playerid, (Vector2){0, 0});
        snakebody->body.back().set_scale(snakebody->scale);
    }
    while (snakebody->body.size() > snakebody->snake->get_body().size()) {
        snakebody->body.pop_back();
    }
    if (snakebody->snake->get_body().size() != 0) {
        snakebody->head[0].set_pos(snakebody->snake->get_body()[0].x * 32, snakebody->snake->get_body()[0].y * 32 + 200);
        snakebody->head[1].set_pos(snakebody->snake->get_body()[0].x * 32, snakebody->snake->get_body()[0].y * 32 + 200);
    }
    for (int i = 0; i < (int)snakebody->body.size(); i++) {
        snakebody->body[i].set_pos(snakebody->snake->get_body()[i].x, snakebody->snake->get_body()[i].y);
    }
    for (int i = 0; i < (int)snakebody->body.size(); i++) {
        SnakeBlock *pre = nullptr, *nxt = nullptr;
        if (i - 1 >= 0) pre = &snakebody->body[i - 1];
        if (i + 1 <= (int)snakebody->body.size() - 1) nxt = &snakebody->body[i + 1];
        snakebody->body[i].set_status(pre, nxt,
            is_speedup & ((i - speedup_offset + speedup_loop_length) % speedup_loop_length < speedup_length),
            game_config().toroidalSpace);
        snakebody->body[i].update();
    }
}

inline void SnakeMove::draw() {
    if (snakebody->snake == nullptr) return;
    for (auto& i : snakebody->body) i.draw();
    snakebody->head[0].draw(); snakebody->head[1].draw();
}

inline void SnakeDie::on_enter() {
    next_state = "";
    pause = false;
    dying_process = 0;
    dying_offset = 0;
    die_block.clear();
}

inline void SnakeDie::update() {
    // 防御：如果 die_block 异常膨胀（超过蛇身长度+dying_length），重置
    if (die_block.size() > snakebody->body.size() + dying_length) {
        die_block.clear();
        dying_offset = 0;
        pause = false;
        dying_process = 0;
    }
    if (pause) {
        // 所有 X 已生成，等待最后一节展示完整的 dying_duration 后切换
        dying_process += GetFrameTime();
        if (dying_process >= dying_duration) {
            next_state = "snake_waiting";
        }
        return;
    }
    dying_process += GetFrameTime();
    while (dying_process >= dying_duration) {
        dying_process -= dying_duration;
        for (int i = dying_offset; i < std::min((int)snakebody->body.size(), dying_offset + dying_length); i++) {
            die_block.emplace_back(std::string("resources/img/die.png"), 5);
            die_block.back().set_scale({2, 2});
            die_block.back().set_pos(snakebody->body[i].get_pos().x * 32, snakebody->body[i].get_pos().y * 32 + 200);
            die_block.back().set_layer(5);
        }
        dying_offset += dying_length;
        if (dying_offset >= (int)snakebody->body.size()) {
            pause = true;
            dying_process = 0;  // 重置计时，给最后一节完整的展示时间
        }
    }
}

inline void SnakeDie::draw() {
    if (snakebody->snake == nullptr) return;
    for (auto& i : snakebody->body) i.draw();
    snakebody->head[0].draw(); snakebody->head[1].draw();
    for (auto& i : die_block) i.draw();
}

inline void SnakeWaiting::draw() {
    if (snakebody->snake == nullptr) return;
    // 虚影模式：绘制半透明身体 / DEATHMATCH幸存者：绘制冻结身体 / 普通死亡：不绘制
    if (!snakebody->snake->is_ghost() && !snakebody->draw_in_waiting) return;
    for (auto& i : snakebody->body) i.draw();
    snakebody->head[0].draw(); snakebody->head[1].draw();
}

inline bool SnakeDie::can_interrupt() const {
    return !pause && interrupt_threshold >= 0
        && dying_offset >= interrupt_threshold
        && dying_offset < (int)snakebody->body.size();
}

inline void SnakeDie::interrupt() {
    dying_offset = (int)snakebody->body.size();
    pause = true;
    dying_process = 0;
}
