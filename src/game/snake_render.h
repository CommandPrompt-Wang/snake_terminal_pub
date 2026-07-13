#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
#include "config/config.h"
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

class Snake_Block : public Basic_Render_Class
{
public:
    Snake_Block (int playerid = 0, Vector2 pos = {0,0})
    :
    playerid(playerid), pos(pos),
    side{Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2),Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2)},
    speedup{Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3),Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3)},
    fill(Sprite("resources/player" + std::to_string(playerid) + "fill.png",1))
    {
        side[2].set_flip_v(1);speedup[2].set_flip_v(1);
        side[3].set_flip_h(1);speedup[3].set_flip_h(1);
    }
    ~Snake_Block (){}

    // Move-only (Sprite 是 move-only 的)
    Snake_Block(Snake_Block&&) = default;
    Snake_Block& operator=(Snake_Block&&) = default;
    Snake_Block(const Snake_Block&) = delete;
    Snake_Block& operator=(const Snake_Block&) = delete;

    void set_status (Snake_Block *pre, Snake_Block *nxt, bool speed_up = false, bool toroidal = false)
    {
        side_status[0] = side_status[1] = side_status[2] = side_status[3] = 0;
        fill.set_hide(0);this->speed_up = speed_up;

        auto check_neighbor = [&](const Snake_Block *neighbor, int i) {
            if (toroidal)
                return is_toroidal_neighbor(pos, neighbor->pos, GRID_W, GRID_H, mv[i].x, mv[i].y);
            else
                return neighbor->pos == pos + mv[i];
        };

        if(pre != nullptr) for (int i = 0; i < 4; i++)
            side_status[i] |= check_neighbor(pre, i);

        if (nxt != nullptr) for (int i = 0; i < 4; i++)
            side_status[i] |= check_neighbor(nxt, i);
    }
    const int get_status()
    {
        int sta = 0;
        for(int i = 0;i < 4;i++)sta = (sta * 10) + (side_status[i]);
        return sta;
    }
    const Vector2 get_pos(){return pos;}
    void set_scale(Vector2 scale)
    {
        this->scale = scale;
        side[0].set_scale(scale);side[1].set_scale(scale);side[2].set_scale(scale);side[3].set_scale(scale);
        speedup[0].set_scale(scale);speedup[1].set_scale(scale);speedup[2].set_scale(scale);speedup[3].set_scale(scale);
        fill.set_scale(scale);
    }

    void set_pos (Vector2 pos)
    {
        this->pos = pos;
        Vector2 draw_position = Vector2{32 * pos.x, 32 * pos.y} + Vector2{0, 200};
        side[0].set_pos(draw_position);side[1].set_pos(draw_position);side[2].set_pos(draw_position);side[3].set_pos(draw_position);
        speedup[0].set_pos(draw_position);speedup[1].set_pos(draw_position);speedup[2].set_pos(draw_position);speedup[3].set_pos(draw_position);
        fill.set_pos(draw_position);
    }
    void set_pos (int x, int y)
    {
        set_pos (Vector2{(float)x, (float)y});
    }

    void draw ()
    {
        side[0].draw();side[1].draw();side[2].draw();side[3].draw();
        speedup[0].draw();speedup[1].draw();speedup[2].draw();speedup[3].draw();
        fill.draw();
    }
    void update ()
    {
        for(int i = 0;i < 4;i++)
        {
            if (side_status[i] == 0)
            {
                if (speed_up)
                {
                    speedup[i].set_hide(0);side[i].set_hide(1);
                }
                else
                {
                    side[i].set_hide(0);speedup[i].set_hide(1);
                }
            }
            else
            {
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
    inline static constexpr Vector2 mv[4] =
    {
        { 0,-1},
        { 1, 0},
        { 0, 1},
        {-1, 0}
    };
};

class Snake_Body : public Basic_Render_Class
{
private:
    Sprite head[2] = {Sprite("resources/up_head.png",10),Sprite("resources/right_head.png",10)};
    std::vector<Snake_Block> body;
    float frame_process = 0;
    inline static const float speedup_time = 0.6; // 加速向后传递的时间
    inline static constexpr int speedup_loop_length = 4, speedup_length = 2;//looplength表示多长一循环，speeduplength表示加速节多长
    int speedup_offset = 0;
    SnakeState* snake;
    int playerid;
    Vector2 scale = {1, 1};//设置缩放
public:
    Snake_Body (SnakeState* snake = nullptr, int playerid = 0) : snake(snake), playerid(playerid)
    {
        speedup_offset = 0;
        frame_process = 0;
    }

    // Move-only (Sprite 是 move-only 的)
    Snake_Body(Snake_Body&&) = default;
    Snake_Body& operator=(Snake_Body&&) = default;
    Snake_Body(const Snake_Body&) = delete;
    Snake_Body& operator=(const Snake_Body&) = delete;
    void set_scale (Vector2 scale)
    {
        this->scale = scale;
        for(auto &i : body)i.set_scale(scale);
        head[0].set_scale(scale);
        head[1].set_scale(scale);
    }
    void update ()
    {
        if(snake == nullptr)return;
        switch (snake->curDir)
        {
            case Direction::UP :
            {
                head[0].set_flip_v(0);head[0].set_hide(0);
                head[1].set_hide(1);
                break;
            }
            case Direction::RIGHT :
            {
                head[1].set_flip_h(0);head[1].set_hide(0);
                head[0].set_hide(1);
                break;
            }
            case Direction::DOWN :
            {
                head[0].set_flip_v(1);head[0].set_hide(0);
                head[1].set_hide(1);
                break;
            }
            case Direction::LEFT :
            {
                head[1].set_flip_h(1);head[1].set_hide(0);
                head[0].set_hide(1);
                break;
            }
        }
        head[0].update();head[1].update();
        bool is_speedup = snake->curSpeed >= 2;
        if (is_speedup)
        {
            frame_process += GetFrameTime();
            while (frame_process > speedup_time)
            {
                frame_process -= speedup_time;
                speedup_offset = (speedup_offset + 1) % speedup_loop_length;
            }
        }
        else
        {
            frame_process = 0;
            speedup_offset = 0;
        }
        while (body.size() < snake->body.size())
        {
            body.emplace_back(playerid, (Vector2){0, 0});
            body.back().set_scale(scale);
        }
        while (body.size() > snake->body.size())
        {
            body.pop_back();
        }
        if(snake->body.size() != 0)
        {
            head[0].set_pos(snake->body[0].x * 32 +    0,snake->body[0].y * 32 + 200);
            head[1].set_pos(snake->body[0].x * 32 +    0,snake->body[0].y * 32 + 200);
        }
        for (int i = 0;i < body.size();i++)
        {
            body[i].set_pos(snake->body[i].x,snake->body[i].y);
        }
        for (int i = 0;i < body.size(); i++)
        {
            Snake_Block *pre = nullptr, *nxt = nullptr;
            if (i - 1 >= 0) pre = &body[i - 1];
            if (i + 1 <= body.size() - 1) nxt = &body[i + 1];
            body[i].set_status(pre, nxt, is_speedup & ((i - speedup_offset + speedup_loop_length) % speedup_loop_length < speedup_length), game_config().toroidalSpace);
            body[i].update();
        }
    }
    void draw ()
    {
        if(snake == nullptr)return;
        for(auto &i : body)i.draw();
        head[0].draw();head[1].draw();
    }
    void print_pos()
    {
        std::cerr << "snake : " << playerid << '\n';
        std::cerr << "head0 = " << head[0].get_hide() << " head1 = " << head[1].get_hide() << '\n';
        for(auto &i : body)
        {
            std::cerr << "##############(" << i.get_pos().x << ", " << i.get_pos().y << ")\n";
            std::cerr << "###############" << i.get_status() << '\n';
        }
    }
    ~Snake_Body ()
    {
        body.clear();
    }
};
