#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
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
    return abs(a.x - b.x) < eps && abs(a.y - b.y) < eps;
}

class Snake_Block : public Basic_Render_Class
{
public:
    Snake_Block (int playerid = 0, Vector2 pos = {0,0})
    :
    playerid(playerid), pos(pos),
    side{Sprite("resources/up_side.png"),Sprite("resources/right_side.png"),Sprite("resources/up_side.png"),Sprite("resources/right_side.png")},
    speedup{Sprite("resources/up_speed_side.png"),Sprite("resources/right_speed_side.png"),Sprite("resources/up_speed_side.png"),Sprite("resources/right_speed_side.png")},
    fill(Sprite("resources/player" + std::to_string(playerid) + "fill.png"))
    {
        side[2].set_flip_v(1);speedup[2].set_flip_v(1);
        side[3].set_flip_h(1);speedup[3].set_flip_h(1);
    }
    ~Snake_Block ()
    {
        fill.set_layer(1);
        for (int i = 0;i < 4;i++)
        {
            side[i].set_layer(1);
            speedup[i].set_layer(2);
        }
    }

    void set_status (Snake_Block *pre, Snake_Block *nxt, bool speed_up = false)
    {
        side_status[0] = side_status[1] = side_status[2] = side_status[3] = 0;
        fill.set_hide(0);this->speed_up = speed_up;

        if(pre != nullptr)for (int i = 0;i < 4;i++)
        {
            side_status[i] |= (pre->pos == pos + mv[i]);
        }

        if (nxt != nullptr)for (int i = 0;i < 4;i++)
        {
            side_status[i] |= (nxt->pos == pos + mv[i]);
        }
    }
    const int get_status()
    {
        int sta = 0;
        for(int i = 0;i < 4;i++)sta = (sta << 1) | (side_status[i]);
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
    Sprite head[2] = {Sprite("resources/up_head.png"),Sprite("resources/right_head.png")};
    std::vector<Snake_Block> body;
    float frame_process = 0;
    inline static const float speedup_time = 0.6; // 加速向后传递的时间
    SnakeState* snake;
    int playerid;
    Vector2 scale = {1, 1};//设置缩放
public:
    Snake_Body (SnakeState* snake = nullptr, int playerid = 0) : snake(snake), playerid(playerid)
    {
        head[0].set_layer(3);
        head[1].set_layer(3);
    }
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
        head[0].update();
        head[1].update();
        while (body.size() < snake->body.size())
        {
            body.emplace_back(playerid, (Vector2){0, 0});
            body.back().set_scale(scale);
        }
        if(snake->body.size() != 0)
        {
            head[0].set_pos(snake->body[0].x,snake->body[0].y);
            head[1].set_pos(snake->body[0].x,snake->body[0].y);
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
            body[i].set_status(pre, nxt);
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
