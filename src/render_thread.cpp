#pragma once

#include "raylib.h"
#include "sprite.h"
#include "event.h"
#include "snake_render.h"
#include "render.h"

#include <list>

class Draw_List
{
public:
    struct DrawEntry
    {
        Basic_Render_Class* item = nullptr;
        bool deleted = false;
    };
    using DrawIter = std::list<DrawEntry>::iterator;
    void update()
    {
        for (DrawIter it = g_draw_list.begin(); it != g_draw_list.end(); )
        {
            if (it->deleted)
            {
                it = g_draw_list.erase(it);
                continue;
            }
            if (it->item != nullptr)
            {
                it->item->update();
            }
            ++it;
        }
        
    }
    void push_back (Basic_Render_Class* item)
    {
        g_draw_list.push_back(DrawEntry{item, false});
    }
    void erase (DrawIter it)
    {
        if(it == g_draw_list.end())return;
        it->deleted = true;
    }
    void clear ()
    {
        g_draw_list.clear();
    }
    void draw()
    {
        for (DrawIter it = g_draw_list.begin(); it != g_draw_list.end(); )
        {
            if (it->deleted)
            {
                it = g_draw_list.erase(it);
                continue;
            }
            if (it->item != nullptr)
            {
                it->item->draw();
            }
            ++it;
        }
    }
private:
    std::list<DrawEntry> g_draw_list;
}draw_list;


inline Color g_clear_color = RAYWHITE;

void render_thread()//画面帧渲染，作为单独的线程
{
    InitWindow(800, 600, "Snake Terminal");
    SetTargetFPS(60);
    int cnt = 0;
    Snake_Block blk = Snake_Block(1, {0,0}, {0,0});
    draw_list.push_back(&blk);
    blk.set_scale({2, 2});
    // Sprite sprite = Sprite("resources/player1fill.png");
    // sprite.set_pos({0, 0});
    // sprite.set_scale({2, 2});
    // draw_list.push_back(&sprite);
    while(!WindowShouldClose()){
        //在这里做画面帧的渲染前准备
        cnt++;
        blk.set_status(nullptr,nullptr,cnt % 30 <= 10);
        draw_list.update();
        BeginDrawing();
        ClearBackground(g_clear_color);
        draw_list.draw();
        EndDrawing();
    }
}
