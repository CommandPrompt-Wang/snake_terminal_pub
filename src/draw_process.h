#pragma once

#include "raylib.h"
#include "sprite.h"
#include "event.h"

#include <list>

// 绘制列表节点：只保存 Sprite*（不持有所有权），支持懒删除标记
// Sprite 的生命周期与资源修改由外部负责
struct DrawEntry
{
    Sprite* sprite = nullptr;  // 非拥有指针，可在运行中由外部修改其资源
    bool deleted = false;      // true：已标记删除，遍历时从 list 摘除且不渲染
};

// 全局绘制链表（STL 双向链表）
inline std::list<DrawEntry> g_draw_list;
inline Color g_clear_color = RAYWHITE;

using DrawIter = std::list<DrawEntry>::iterator;

// 尾插：仅记录指针，不接管所有权
inline DrawIter draw_list_push_back(Sprite* sprite)
{
    if (sprite == nullptr)
    {
        return g_draw_list.end();
    }
    g_draw_list.push_back(DrawEntry{sprite, false});
    DrawIter it = g_draw_list.end();
    --it;
    return it;
}

// 头插：仅记录指针，不接管所有权
inline DrawIter draw_list_push_front(Sprite* sprite)
{
    if (sprite == nullptr)
    {
        return g_draw_list.end();
    }
    g_draw_list.push_front(DrawEntry{sprite, false});
    return g_draw_list.begin();
}

// 懒删除：只打标记，真正从 list 摘除在 draw_process 遍历时进行（不 delete Sprite）
inline void draw_list_erase(DrawIter it)
{
    if (it == g_draw_list.end())
    {
        return;
    }
    it->deleted = true;
}

// 按资源指针懒删除（不 delete Sprite）
inline bool draw_list_erase_resource(Sprite* sprite)
{
    if (sprite == nullptr)
    {
        return false;
    }
    for (DrawIter it = g_draw_list.begin(); it != g_draw_list.end(); ++it)
    {
        if (it->sprite == sprite)
        {
            it->deleted = true;
            return true;
        }
    }
    return false;
}

// 立即清空列表中的指针记录（不销毁 Sprite 对象）
inline void draw_list_clear()
{
    g_draw_list.clear();
}

// 渲染一个画面帧。应在主循环中每帧调用一次。
inline void draw_process()
{
    // 更新：遇到删除标记则从 list 摘除，不 update
    for (DrawIter it = g_draw_list.begin(); it != g_draw_list.end(); )
    {
        if (it->deleted)
        {
            it = g_draw_list.erase(it);
            continue;
        }
        if (it->sprite != nullptr)
        {
            it->sprite->update();
        }
        ++it;
    }

    BeginDrawing();
    ClearBackground(g_clear_color);

    // 绘制：遇到删除标记则从 list 摘除，不渲染
    for (DrawIter it = g_draw_list.begin(); it != g_draw_list.end(); )
    {
        if (it->deleted)
        {
            it = g_draw_list.erase(it);
            continue;
        }
        if (it->sprite != nullptr)
        {
            it->sprite->draw();
        }
        ++it;
    }

    EndDrawing();
}
