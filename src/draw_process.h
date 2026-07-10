#pragma once

#include "raylib.h"
#include "sprite.h"
#include "draw_list.h"
#include "event.h"

// 全局绘制链表：每个节点维护唯一一份 Sprite
inline DrawList g_draw_list;
inline Color g_clear_color = RAYWHITE;

// 渲染一个画面帧。应在主循环中每帧调用一次。
inline void draw_process()
{
    // 推进精灵表动画
    for (DrawNode* node = g_draw_list.head; node != nullptr; node = node->next)
    {
        if (node->resource != nullptr)
        {
            node->resource->update();
        }
    }

    BeginDrawing();
    ClearBackground(g_clear_color);

    // 按链表顺序从头到尾绘制
    for (DrawNode* node = g_draw_list.head; node != nullptr; node = node->next)
    {
        if (node->resource != nullptr)
        {
            node->resource->draw();
        }
    }

    EndDrawing();
}
