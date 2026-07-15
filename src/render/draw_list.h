#pragma once

#include "raylib.h"
#include "global.h"
#include "render/render.h"

#include <list>
#include <iostream>
#include <algorithm>


class Draw_By_Layer
{
    public:
    struct data
    {
        Texture2D texture; Rectangle source; Rectangle dest; Vector2 origin; float rotation; Color tint;
    };
    std::vector<std::pair<data,std::pair<int,std::string> > > vec;
    void push_draw(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, int layer,std::string imagepath = "")
    {
        vec.emplace_back(data{texture,source,dest,origin,rotation,tint},std::make_pair(layer,imagepath));
    }
    void draw()
    {
        std::sort(vec.begin(),vec.end(),[](auto& a,auto& b){return a.second.first < b.second.first;});
        for (auto &i : vec)
        {
            DrawTexturePro(i.first.texture,i.first.source,i.first.dest,i.first.origin,i.first.rotation,i.first.tint);
            // std::cerr << "asd;lkjfaj;lkkj;flsdaj;klfsdaafsd; i.second = " << i.second.first << " i.path = " << i.second.second <<  '\n';
        }
        vec.clear();
    }
};

inline Draw_By_Layer draw_layer;
inline void push_draw(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, int layer, std::string imagepath = "")
{
    draw_layer.push_draw(texture, source, dest, origin, rotation, tint, layer, imagepath);
}

class Draw_List
{
public:
    struct DrawEntry
    {
        BasicRenderClass* item = nullptr;
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
    void push_back (BasicRenderClass* item)
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
        draw_layer.draw();
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
    std::list<DrawEntry> g_draw_list;
};
