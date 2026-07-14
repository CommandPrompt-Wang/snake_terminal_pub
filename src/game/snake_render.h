#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
#include "config/config.h"
#include <string>
#include <array>
#include <vector>
#include <map>

class AnimateState : public Basic_Render_Class
{
protected:
    std::string state_name;
public:
    std::string next_state;
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual void on_exit() = 0;
    virtual void on_enter() = 0;
    AnimateState(const std::string& state_name) : state_name(state_name) {}
    AnimateState() = default;
    virtual ~AnimateState() = default;
};

class AnimateManager : public Basic_Render_Class
{
private:
    std::map<std::string, AnimateState*> mp;
public:
    std::string current_state = "";
    AnimateManager() = default;
    ~AnimateManager() { mp.clear(); }

    void update()
    {
        if (current_state.empty()) return;
        auto it = mp.find(current_state);
        if (it == mp.end() || it->second == nullptr) return;
        it->second->update();
        if (!it->second->next_state.empty())
        {
            switch_state(it->second->next_state);
        }
    }

    void draw()
    {
        if (current_state.empty()) return;
        auto it = mp.find(current_state);
        if (it == mp.end() || it->second == nullptr) return;
        it->second->draw();
    }

    void add_state(const std::string& state_name, AnimateState& state)
    {
        mp[state_name] = &state;
    }

    void switch_state(const std::string& state_name)
    {
        if (current_state == state_name || mp.find(state_name) == mp.end()) return;
        if (!current_state.empty())
        {
            auto it = mp.find(current_state);
            if (it != mp.end() && it->second != nullptr)
            {
                it->second->on_exit();
            }
        }
        auto it = mp.find(state_name);
        if (it != mp.end() && it->second != nullptr)
        {
            current_state = state_name;
            it->second->on_enter();
        }
    }
};

constexpr float eps = 1e-6;

inline Vector2 operator + (const Vector2& a,const Vector2& b)
{
    return {a.x + b.x,a.y + b.y};
}
inline bool operator == (const Vector2& a,const Vector2& b)
{
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps;
}

// 在环形空间中检查 (center + offset) 是否等于 candidate（考虑边界卷绕）
inline bool is_toroidal_neighbor(const Vector2& center, const Vector2& candidate,
                                 int gridW, int gridH, float dx, float dy) {
    int cx = (int)center.x, cy = (int)center.y;
    int nx = (int)candidate.x, ny = (int)candidate.y;
    int ex = (cx + (int)dx + gridW) % gridW;
    int ey = (cy + (int)dy + gridH) % gridH;
    return nx == ex && ny == ey;
}

class SnakeBlock : public Basic_Render_Class
{
public:
    SnakeBlock (int playerid = 0, Vector2 pos = {0,0})
    :
    playerid(playerid), pos(pos),
    side{Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2),Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2)},
    speedup{Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3),Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3)},
    fill(Sprite("resources/player" + std::to_string(playerid) + "fill.png",1)) {
        side[2].set_flip_v(1);speedup[2].set_flip_v(1);
        side[3].set_flip_h(1);speedup[3].set_flip_h(1);
    }
    ~SnakeBlock (){}

    // Move-only (Sprite 是 move-only 的)
    SnakeBlock(SnakeBlock&&) = default;
    SnakeBlock& operator=(SnakeBlock&&) = default;
    SnakeBlock(const SnakeBlock&) = delete;
    SnakeBlock& operator=(const SnakeBlock&) = delete;

    void set_status (SnakeBlock *pre, SnakeBlock *nxt, bool speed_up = false, bool toroidal = false) {
        side_status[0] = side_status[1] = side_status[2] = side_status[3] = 0;
        fill.set_hide(0);this->speed_up = speed_up;

        auto check_neighbor = [&](const SnakeBlock *neighbor, int i) {
            if (toroidal)
                return is_toroidal_neighbor(pos, neighbor->pos, GRID_W, GRID_H, mv[i].x, mv[i].y);
            else
                return neighbor->pos == pos + mv[i];
        };

        if(pre != nullptr) for (int i = 0; i < 4; i++) {
            side_status[i] |= check_neighbor(pre, i);
        }

        if (nxt != nullptr) {
            for (int i = 0; i < 4; i++)
                side_status[i] |= check_neighbor(nxt, i);
        }
    }
    void set_hide(bool hide){is_hide = hide;}
    const int get_status() {
        int sta = 0;
        for(int i = 0;i < 4;i++)sta = (sta * 10) + (side_status[i]);
        return sta;
    }
    const Vector2 get_pos(){return pos;}
    void set_scale(Vector2 scale) {
        this->scale = scale;
        side[0].set_scale(scale);side[1].set_scale(scale);side[2].set_scale(scale);side[3].set_scale(scale);
        speedup[0].set_scale(scale);speedup[1].set_scale(scale);speedup[2].set_scale(scale);speedup[3].set_scale(scale);
        fill.set_scale(scale);
    }

    void set_pos (Vector2 pos) {
        this->pos = pos;
        Vector2 draw_position = Vector2{32 * pos.x, 32 * pos.y} + Vector2{0, 200};
        side[0].set_pos(draw_position);side[1].set_pos(draw_position);side[2].set_pos(draw_position);side[3].set_pos(draw_position);
        speedup[0].set_pos(draw_position);speedup[1].set_pos(draw_position);speedup[2].set_pos(draw_position);speedup[3].set_pos(draw_position);
        fill.set_pos(draw_position);
    }
    void set_pos (int x, int y) {
        set_pos (Vector2{(float)x, (float)y});
    }

    void draw () {
        if (is_hide == true)return;
        side[0].draw();side[1].draw();side[2].draw();side[3].draw();
        speedup[0].draw();speedup[1].draw();speedup[2].draw();speedup[3].draw();
        fill.draw();
    }
    void update () {
        for (int i = 0; i < 4; i++) {
            if (side_status[i] == 0) {
                if (speed_up) {
                    speedup[i].set_hide(0);side[i].set_hide(1);
                } else {
                    side[i].set_hide(0);speedup[i].set_hide(1);
                }
            } else {
                speedup[i].set_hide(1);side[i].set_hide(1);
            }
        }
        side[0].update();side[1].update();side[2].update();side[3].update();
        speedup[0].update();speedup[1].update();speedup[2].update();speedup[3].update();
        fill.update();
    }
private:
    std::array<Sprite, 4> side;
    std::array<Sprite, 4> speedup;
    Sprite fill;

    bool speed_up = false;

    Vector2 pos{0, 0};
    Vector2 scale{1, 1};
    int playerid = 0;
    // pos 为逻辑网格坐标，实际像素位置 = pos * CELL_SIZE + OFFSET
    // 地图以左上角为原点，向右为 x+，向下为 y+
    bool side_status[4] = {0, 0, 0, 0}; // 0=上,1=右,2=下,3=左; 0=显示侧边,1=隐藏
    bool is_hide = false;
    inline static constexpr Vector2 mv[4] =
    {
        { 0,-1},
        { 1, 0},
        { 0, 1},
        {-1, 0}
    };
};
class SnakeBody;
class SnakeMove : public AnimateState
{
private:
    Snake* snake;
    int playerid = 0;
    Vector2 scale_ = {1, 1};
    std::array<Sprite, 2> head;
    std::vector<SnakeBlock> body;
    float frame_process = 0.0f;
    inline static const float speedup_duration = 0.6f;
    inline static constexpr int speedup_loop_length = 4, speedup_length = 2;
    int speedup_offset = 0;
public:
    explicit SnakeMove(Snake* snake, int playerid = 0)
        : snake(snake)
        , playerid(playerid)
        , head{Sprite("resources/up_head.png", 10), Sprite("resources/right_head.png", 10)}
    {}

    SnakeMove(const SnakeMove&) = delete;
    SnakeMove& operator=(const SnakeMove&) = delete;
    SnakeMove(SnakeMove&&) = default;
    SnakeMove& operator=(SnakeMove&&) = default;

    void set_scale(Vector2 scale)
    {
        scale_ = scale;
        for (auto& i : body) i.set_scale(scale);
        head[0].set_scale(scale);
        head[1].set_scale(scale);
    }

    void set_playerid(int id)
    {
        playerid = id;
    }

    std::vector<SnakeBlock>& get_body()
    {
        return body;
    }

    std::array<Sprite, 2>& get_head()
    {
        return head;
    }

    void update()
    {
        if (snake == nullptr) return;

        switch (snake->get_direction()) {
            case Direction::UP: {
                head[0].set_flip_v(0);head[0].set_hide(0);
                head[1].set_hide(1);
                break;
            }
            case Direction::RIGHT: {
                head[1].set_flip_h(0);head[1].set_hide(0);
                head[0].set_hide(1);
                break;
            }
            case Direction::DOWN: {
                head[0].set_flip_v(1);head[0].set_hide(0);
                head[1].set_hide(1);
                break;
            }
            case Direction::LEFT: {
                head[1].set_flip_h(1);head[1].set_hide(0);
                head[0].set_hide(1);
                break;
            }
        }
        head[0].update();head[1].update();
        bool is_speedup = snake->get_speed() >= 2;
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
        while (body.size() < snake->get_body().size()) {
            body.emplace_back(playerid, (Vector2){0, 0});
            body.back().set_scale(scale_);
        }
        while (body.size() > snake->get_body().size()) {
            body.pop_back();
        }
        if (snake->get_body().size() != 0) {
            const auto& pos = snake->get_body()[0];
            head[0].set_pos(pos.x * 32.0f, pos.y * 32.0f + 200.0f);
            head[1].set_pos(pos.x * 32.0f, pos.y * 32.0f + 200.0f);
        }
        for (int i = 0; i < (int)body.size(); i++) {
            body[i].set_pos(snake->get_body()[i].x, snake->get_body()[i].y);
        }
        for (int i = 0; i < (int)body.size(); i++) {
            SnakeBlock *pre = nullptr, *nxt = nullptr;
            if (i - 1 >= 0) pre = &body[i - 1];
            if (i + 1 <= (int)body.size() - 1) nxt = &body[i + 1];
            body[i].set_status(pre, nxt, is_speedup & ((i - speedup_offset + speedup_loop_length) % speedup_loop_length < speedup_length), game_config().toroidalSpace);
            body[i].update();
        }
    }
    void on_enter()
    {
        speedup_offset = 0;
        frame_process = 0;
        next_state = "";
    }
    void draw()
    {
        if(snake == nullptr)return;
        for (auto &i : body) i.draw();
        head[0].draw();head[1].draw();
    }
    void on_exit(){}
};

class SnakeDie : public AnimateState
{
private:
    SnakeMove* snakemove;
    Snake* snake;
    bool pause = false;
    std::vector<Sprite> die_block;
    Vector2 scale_ = {1, 1};
    float dying_process = 0.0f;
    static constexpr float dying_duration = 0.3f;
    inline static constexpr int dying_length = 1;
    int dying_offset = 0;
public:
    SnakeDie(SnakeMove* snakemove, Snake* snake)
        : snakemove(snakemove), snake(snake) {}

    SnakeDie(const SnakeDie&) = delete;
    SnakeDie& operator=(const SnakeDie&) = delete;
    SnakeDie(SnakeDie&&) = default;
    SnakeDie& operator=(SnakeDie&&) = default;

    ~SnakeDie(){}

    void on_enter()
    {
        next_state = "";
        pause = false;
        dying_process = 0;
        dying_offset = 0;
        die_block.clear();
    }
    void update()
    {
        if (pause)
        {
            on_exit();return;
        }
        if (snake == nullptr || snakemove == nullptr) return;

        auto& body = snakemove->get_body();
        dying_process += GetFrameTime();
        while (dying_process >= dying_duration)
        {
            dying_process -= dying_duration;
            for (int i = dying_offset; i < std::min((int)body.size(), dying_offset + dying_length); i++)
            {
                die_block.emplace_back("resources/die.png", 5);
                die_block.back().set_scale({2,2});
                die_block.back().set_pos(body[i].get_pos().x * 32.0f, body[i].get_pos().y * 32.0f + 200.0f);
                die_block.back().set_layer(5);
            }
            dying_offset += dying_length;
            if (dying_offset >= (int)body.size())
            {
                next_state = "";
            }
        }
    }
    void draw()
    {
        if (snake == nullptr || snakemove == nullptr) return;
        auto& body = snakemove->get_body();
        auto& head = snakemove->get_head();
        for (auto &i : body) i.draw();
        head[0].draw();head[1].draw();
        for (auto &i : die_block) i.draw();
    }
    void on_exit()
    {
        if (snake != nullptr) {
            snake->set_animation_status(Snake::AnimationStatus::WAITING);
        }
    }
};

class SnakeBody : public Basic_Render_Class
{
    friend class SnakeMove;
    friend class SnakeDie;
protected:
    Snake* snake;
    int playerid;
    bool is_hide = 0;
    Vector2 scale = {1, 1};

    AnimateManager animate_manager;
    SnakeMove snakemove;
    SnakeDie snakedie;
public:
    SnakeBody (Snake* snake = nullptr, int playerid = 0)
        : snake(snake), playerid(playerid), snakemove(snake, playerid), snakedie(&snakemove, snake)
    {
        // std::cerr << "trying to add state" << "\n";
        animate_manager.add_state("snake_move",snakemove);
        animate_manager.add_state("snake_die",snakedie);
        animate_manager.switch_state("snake_move");
    }

    // Move-only (Sprite 是 move-only 的)
    SnakeBody(SnakeBody&&) = default;
    SnakeBody& operator=(SnakeBody&&) = default;
    SnakeBody(const SnakeBody&) = delete;
    SnakeBody& operator=(const SnakeBody&) = delete;
    void set_scale (Vector2 scale) {
        this->scale = scale;
        snakemove.set_scale(scale);
    }
    void set_hide (bool hide){is_hide = hide;}

    void update () {
        std::cerr << "updating\n";
        if(snake == nullptr)return;
        if(snake->get_animation_status() ==  Snake::AnimationStatus::DYING)
        {
            animate_manager.switch_state("snake_die");
        }
        std::cerr << "Current State is :" << animate_manager.current_state << '\n';
        animate_manager.update();
    }
    void draw () {
        if (snake == nullptr || is_hide == true) return;
        animate_manager.draw();
    }
    void print_pos() {
        std::cerr << "snake : " << playerid << '\n';
        std::cerr << "head0 = " << snakemove.get_head()[0].get_hide() << " head1 = " << snakemove.get_head()[1].get_hide() << '\n';
        for (auto &i : snakemove.get_body()) {
            std::cerr << "##############(" << i.get_pos().x << ", " << i.get_pos().y << ")\n";
            std::cerr << "###############" << i.get_status() << '\n';
        }
    }
    ~SnakeBody () {
    }
};
