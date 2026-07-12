#pragma once
#include "raylib.h"
#include "sprite.h"
#include "render.h"
#include<string>

Vector2 operator + (const Vector2& a,const Vector2& b)
{
    return {a.x + b.x,a.y + b.y};
}
bool operator == (const Vector2& a,const Vector2& b)
{
    return a.x == b.x && a.y == b.y;
}

class Snake_Block : Basic_Render_Class
{
public:
    Snake_Block (int playerid = 0, Vector2 pos = {0,0}, Vector2 offset = {0,0})//color = false : player 1; color = true : player 2
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

    void set_status (Snake_Block *pre, Snake_Block *nxt, bool speed_up = false)//更新边缘显示状态和头部显示状态
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
            Event dir = EventServer::getEvent(playerid, EventType::MOVE);
            switch (dir)
            {
                case Event::UP :
                {
                    head[0].set_pos(draw_position);
                    head[0].set_flip_v(0);head[0].set_hide(0);
                    head[0].update();head[0].draw();
                    break;
                }
                case Event::RIGHT :
                {
                    head[1].set_pos(draw_position);
                    head[1].set_flip_h(0);head[1].set_hide(0);
                    head[1].update();head[1].draw();
                    break;
                }
                case Event::DOWN :
                {
                    head[0].set_pos(draw_position);
                    head[0].set_flip_v(1);head[0].set_hide(0);
                    head[0].update();head[0].draw();
                    break;
                }
                case Event::LEFT :
                {
                    head[1].set_pos(draw_position);
                    head[1].set_flip_h(1);head[1].set_hide(0);
                    head[1].update();head[1].draw();
                    break;
                }
                default:break;
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

    Vector2 pos;
    int playerid;
    //这里的pos表示在地图的pos，乘以固定比率才是实际的位置
    //地图以左上角为原点，向右为x+，向下为y+
    bool side_status[4];//0 = up, 1 = right, 2 = down, 3 = left;side = 0 for show, 1 for hide
    static constexpr Vector2 mv[4] =
    {
        { 0,-1},
        { 1, 0},
        { 0, 1},
        {-1, 0}
    };
};

class Snake_Body : Basic_Render_Class
{
    
    void update ()
    {}
    void draw ()
    {}
};
