#pragma once

#include "raylib.h"
#include "render/sprite.h"
#include "global.h"
#include "render/render.h"

#include <list>
#include <iostream>


class Draw_By_Layer
{
    public:
    struct data
    {
        Texture2D texture; Rectangle source; Rectangle dest; Vector2 origin; float rotation; Color tint;
    };
    std::vector<std::pair<data,int> > vec;
    void push_draw(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, int layer)
    {
        vec.emplace_back(data{texture,source,dest,origin,rotation,tint},layer);
    }
    void draw()
    {
        std::sort(vec.begin(),vec.end(),[](auto& a,auto& b){return a.second > b.second;});
        for(auto &i : vec)DrawTexturePro(i.first.texture,i.first.source,i.first.dest,i.first.origin,i.first.rotation,i.first.tint);
        vec.clear();
    }
}draw_layer;
void push_draw(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, int layer)
{
    draw_layer.push_draw(texture, source, dest, origin, rotation, tint, layer);
}

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
