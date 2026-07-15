#pragma once

#include "raylib.h"
#include "global.h"
#include "render/render.h"

#include <list>
#include <algorithm>


// 图层绘制单例：收集每帧绘制命令，按 layer 排序后批量 DrawTexturePro
class DrawLayer {
public:
    struct Item {
        Texture2D texture; Rectangle source; Rectangle dest; Vector2 origin; float rotation; Color tint;
    };

    static void push(Texture2D texture, Rectangle source, Rectangle dest,
                     Vector2 origin, float rotation, Color tint, int layer,
                     std::string imagepath = "") {
        auto& self = instance();
        self.vec.emplace_back(Item{texture, source, dest, origin, rotation, tint},
                              std::make_pair(layer, imagepath));
    }

    static void flush() {
        auto& self = instance();
        std::sort(self.vec.begin(), self.vec.end(),
                  [](auto& a, auto& b) { return a.second.first < b.second.first; });
        for (auto& i : self.vec)
            DrawTexturePro(i.first.texture, i.first.source, i.first.dest,
                           i.first.origin, i.first.rotation, i.first.tint);
        self.vec.clear();
    }

private:
    static DrawLayer& instance() {
        static DrawLayer layer;
        return layer;
    }
    std::vector<std::pair<Item, std::pair<int, std::string>>> vec;
};

class Draw_List
{
public:
    struct DrawEntry
    {
        BasicRenderClass* item = nullptr;
        bool deleted = false;
    };
    using iterator = std::list<DrawEntry>::iterator;
    void update()
    {
        for (auto it = draw_entries_.begin(); it != draw_entries_.end(); )
        {
            if (it->deleted)
            {
                it = draw_entries_.erase(it);
                continue;
            }
            if (it->item != nullptr)
            {
                it->item->update();
            }
            ++it;
        }
        
    }
    void push_back (BasicRenderClass* item)
    {
        draw_entries_.push_back(DrawEntry{item, false});
    }
    void erase (iterator it)
    {
        if(it == draw_entries_.end())return;
        it->deleted = true;
    }
    void clear ()
    {
        draw_entries_.clear();
    }
    void draw()
    {

        for (auto it = draw_entries_.begin(); it != draw_entries_.end(); )
        {
            if (it->deleted)
            {
                it = draw_entries_.erase(it);
                continue;
            }
            if (it->item != nullptr)
            {
                it->item->draw();
            }
            ++it;
        }
        DrawLayer::flush();
    }
    struct texturearg
    {
        Texture2D texture;
        Rectangle source, dest;
        Vector2 origin;
        float rotation;
        Color tint;
    };
private:
    std::list<DrawEntry> draw_entries_;
};
