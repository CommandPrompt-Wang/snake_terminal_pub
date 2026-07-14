#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
#include "config/config.h"
#include "game/snake_state/animate_manager.h"
#include <string>
#include <array>
#include <vector>

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
    SnakeBody* snakebody;
public:
    SnakeMove(SnakeBody* snakebody):snakebody(snakebody){}

    void update()
    {
        switch (snakebody->snake->get_direction()) {
            case Direction::UP: {
                snakebody->head[0].set_flip_v(0);snakebody->head[0].set_hide(0);
                snakebody->head[1].set_hide(1);
                break;
            }
            case Direction::RIGHT: {
                snakebody->head[1].set_flip_h(0);snakebody->head[1].set_hide(0);
                snakebody->head[0].set_hide(1);
                break;
            }
            case Direction::DOWN: {
                snakebody->head[0].set_flip_v(1);snakebody->head[0].set_hide(0);
                snakebody->head[1].set_hide(1);
                break;
            }
            case Direction::LEFT: {
                snakebody->head[1].set_flip_h(1);snakebody->head[1].set_hide(0);
                snakebody->head[0].set_hide(1);
                break;
            }
        }
        snakebody->head[0].update();snakebody->head[1].update();
        bool is_speedup = snakebody->snake->get_speed() >= 2;
        if (is_speedup) {
            snakebody->frame_process += GetFrameTime();
            while (snakebody->frame_process > snakebody->speedup_duration) {
                snakebody->frame_process -= snakebody->speedup_duration;
                snakebody->speedup_offset = (snakebody->speedup_offset + 1) % snakebody->speedup_loop_length;
            }
        } else {
            snakebody->frame_process = 0;
            snakebody->speedup_offset = 0;
        }
        while (snakebody->body.size() < snakebody->snake->get_body().size()) {
            snakebody->body.emplace_back(snakebody->playerid, (Vector2){0, 0});
            snakebody->body.back().set_scale(snakebody->scale);
        }
        while (snakebody->body.size() > snakebody->snake->get_body().size()) {
            snakebody->body.pop_back();
        }
        if (snakebody->snake->get_body().size() != 0) {
            snakebody->head[0].set_pos(snakebody->snake->get_body()[0].x * 32 + 0,snakebody->snake->get_body()[0].y * 32 + 200);
            snakebody->head[1].set_pos(snakebody->snake->get_body()[0].x * 32 + 0,snakebody->snake->get_body()[0].y * 32 + 200);
        }
        for (int i = 0; i < snakebody->body.size(); i++) {
            snakebody->body[i].set_pos(snakebody->snake->get_body()[i].x,snakebody->snake->get_body()[i].y);
        }
        for (int i = 0; i < snakebody->body.size(); i++) {
            SnakeBlock *pre = nullptr, *nxt = nullptr;
            if (i - 1 >= 0) pre = &snakebody->body[i - 1];
            if (i + 1 <= (int)snakebody->body.size() - 1) nxt = &snakebody->body[i + 1];
            snakebody->body[i].set_status(pre, nxt, is_speedup & ((i - snakebody->speedup_offset + snakebody->speedup_loop_length) % snakebody->speedup_loop_length < snakebody->speedup_length), game_config().toroidalSpace);
            snakebody->body[i].update();
        }
    }
    void on_enter()
    {
        snakebody->speedup_offset = 0;
        snakebody->frame_process = 0;
        next_state = "";
    }
    void draw()
    {
        if(snakebody->snake == nullptr)return;
        for (auto &i : snakebody->body) i.draw();
        snakebody->head[0].draw();snakebody->head[1].draw();
    }
    void on_exit(){}
};

class SnakeDie : public AnimateState
{
private:
    SnakeBody* snakebody;
    bool pause;
public:
    SnakeDie(SnakeBody* snakebody):snakebody(snakebody){}
    ~SnakeDie(){}
    std::vector<Sprite> die_block;
    void on_enter()
    {
        next_state = "";
        pause = 0;
        snakebody->dying_process = 0;
        snakebody->dying_offset = 0;
    }
    void update()
    {
        if(pause)
        {
            on_exit();return;
        }
        snakebody->dying_process += GetFrameTime();
        while(snakebody->dying_process >= snakebody->dying_duration)
        {
            snakebody->dying_process -= snakebody->dying_duration;
            for(int i = snakebody->dying_offset;i < std::min((int)snakebody->body.size(),snakebody->dying_offset + snakebody->dying_length);i++)
            {
                die_block.emplace_back("resources/die.png");
                die_block.back().set_scale({2,2});
                die_block.back().set_pos(snakebody->body[i].get_pos().x * 32,snakebody->body[i].get_pos().y * 32 + 200);
                die_block.back().set_layer(5);
            }
            snakebody->dying_offset += snakebody->dying_length;
            if(snakebody->dying_offset >= snakebody->body.size())
            {
                next_state = "";
            }
        }
    }
    void draw()
    {
        if(snakebody->snake == nullptr)return;
        for (auto &i : snakebody->body) i.draw();
        snakebody->head[0].draw();snakebody->head[1].draw();
        for(auto &i : die_block)i.draw();
    }
    void on_exit()
    {
        snakebody->snake->set_animation_status(Snake::AnimationStatus::WAITING);
    }
};

class SnakeBody : public Basic_Render_Class
{
    friend class SnakeMove;
    friend class SnakeDie;
protected:
    Sprite head[2] = {Sprite("resources/up_head.png",10),Sprite("resources/right_head.png",10)};
    std::vector<SnakeBlock> body;
    float frame_process = 0;
    inline static const float speedup_duration = 0.6; // 加速向后传递的时间
    inline static constexpr int speedup_loop_length = 4, speedup_length = 2;//looplength表示多长一循环，speeduplength表示加速节多长
    int speedup_offset = 0;
    Snake* snake;
    int playerid;
    bool is_hide = 0;
    Vector2 scale = {1, 1};//设置缩放

    // 死亡动画计时（渲染层自行管理）
    float dying_process = 0;
    static constexpr float dying_duration = 0.3f;//死亡动画向后传递的时间
    inline static constexpr int dying_length = 1;//dyinglength表示一次传多长的死亡节
    int dying_offset = 0;

    AnimateManager animate_manager;
    SnakeMove snakemove;
    SnakeDie snakedie;
public:
    SnakeBody (Snake* snake = nullptr, int playerid = 0) : snake(snake), playerid(playerid),snakemove(this),snakedie(this){
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
        for(auto &i : body)i.set_scale(scale);
        head[0].set_scale(scale);
        head[1].set_scale(scale);
    }
    void set_hide (bool hide){is_hide = hide;}

    void update () {
        if(snake == nullptr)return;
        if(snake->get_animation_status() ==  Snake::AnimationStatus::DYING)
        {
            animate_manager.switch_state("snake_die");
        }
        animate_manager.update();
    }
    void draw () {
        if (snake == nullptr || is_hide == true) return;
        animate_manager.draw();
    }
    void print_pos() {
        std::cerr << "snake : " << playerid << '\n';
        std::cerr << "head0 = " << head[0].get_hide() << " head1 = " << head[1].get_hide() << '\n';
        for (auto &i : body) {
            std::cerr << "##############(" << i.get_pos().x << ", " << i.get_pos().y << ")\n";
            std::cerr << "###############" << i.get_status() << '\n';
        }
    }
    ~SnakeBody () {
    }
};
