#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
#include<string>

inline Vector2 operator + (const Vector2& a,const Vector2& b)
{
    return {a.x + b.x,a.y + b.y};
}
inline bool operator == (const Vector2& a,const Vector2& b)
{
    return a.x == b.x && a.y == b.y;
}

class Snake_Block : public Basic_Render_Class
{
public:
    Snake_Block (int playerid = 0, Vector2 pos = {0,0}, Vector2 offset = {0,0})
    :
    side({Sprite("resources/up_side.png"),Sprite("resources/right_side.png"),Sprite("resources/up_side.png"),Sprite("resources/right_side.png")}),
    speedup({Sprite("resources/up_speed_side.png"),Sprite("resources/right_speed_side.png"),Sprite("resources/up_speed_side.png"),Sprite("resources/right_speed_side.png")}),
    fill(Sprite("resources/player" + std::to_string(playerid) + "fill.png"))
    {
        side[2].set_flip_v(1);speedup[2].set_flip_v(1);
        side[3].set_flip_h(1);speedup[3].set_flip_h(1);
    }
    ~Snake_Block ()
    {
    }

    void set_status (Snake_Block *pre, Snake_Block *nxt, bool speed_up = false)
    {
        side_status[0] = side_status[1] = side_status[2] = side_status[3] = 0;
        fill.set_hide(0);is_head = false;this->speed_up = speed_up;

        if (pre == nullptr)
        {
            is_head = true;
        }
        else for (int i = 0;i < 4;i++)
        {
            side_status[i] |= (pre->pos == pos + mv[i]);
        }

        if (nxt != nullptr)
        {
            for (int i = 0;i < 4;i++)
            {
                side_status[i] |= (nxt->pos == pos + mv[i]);
            }
        }
    }

    void set_dir(Direction d) { dir_ = d; }
    void set_pos (Vector2 pos)
    {
        this->pos = pos;
    }
    void draw ()
    {
        Vector2 draw_position = pos;
        fill.set_pos(draw_position); fill.draw();
        if (is_head)
        {
            switch (dir_)
            {
                case Direction::UP :
                {
                    head[0].set_pos(draw_position);
                    head[0].set_flip_v(0);head[0].set_hide(0);
                    head[0].update();head[0].draw();
                    break;
                }
                case Direction::RIGHT :
                {
                    head[1].set_pos(draw_position);
                    head[1].set_flip_h(0);head[1].set_hide(0);
                    head[1].update();head[1].draw();
                    break;
                }
                case Direction::DOWN :
                {
                    head[0].set_pos(draw_position);
                    head[0].set_flip_v(1);head[0].set_hide(0);
                    head[0].update();head[0].draw();
                    break;
                }
                case Direction::LEFT :
                {
                    head[1].set_pos(draw_position);
                    head[1].set_flip_h(1);head[1].set_hide(0);
                    head[1].update();head[1].draw();
                    break;
                }
            }
        }
        for(int i = 0;i < 4;i++)
        {
            if (side_status[i] == 0)
            {
                if (speed_up)
                {
                    speedup[i].set_pos(draw_position);
                    speedup[i].set_hide(0);
                    speedup[i].update();speedup[i].draw();
                }
                else
                {
                    side[i].set_pos(draw_position);
                    side[i].set_hide(0);
                    side[i].update();side[i].draw();
                }
            }
        }
    }
    void update ()
    {
        head[0].update();head[1].update();
        side[0].update();side[1].update();side[2].update();side[3].update();
        speedup[0].update();speedup[1].update();speedup[2].update();speedup[3].update();
        fill.update();
    }
private:
    Sprite side[4];
    Sprite speedup[4];
    Sprite fill;

    inline static Sprite head[2] = {Sprite("resources/up_head.png"),Sprite("resources/right_head.png")};
    bool is_head = false,speed_up = false;
    Direction dir_ = Direction::DOWN;

    Vector2 pos;
    int playerid;
    // Map coordinates, origin top-left, x+ right, y+ down
    bool side_status[4];//0 = up, 1 = right, 2 = down, 3 = left; 0 = show, 1 = hide
    static constexpr Vector2 mv[4] =
    {
        { 0,-1},
        { 1, 0},
        { 0, 1},
        {-1, 0}
    };
};

class Snake_Body : public Basic_Render_Class
{
    void update ()
    {}
    void draw ()
    {}
};
