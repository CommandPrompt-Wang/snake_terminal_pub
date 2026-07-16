#pragma once

#include "global.h"
#include "raylib.h"
#include "render/render.h"

#include <algorithm>
#include <list>

// 图层绘制单例：收集每帧 DrawTexturePro 命令，在 flush 时按 layer 升序批量提交。
// Sprite 等对象只负责 push，不直接调用 raylib 绘制，以保证同层/跨层顺序可控。
class DrawLayer {
public:
    // 单次纹理绘制参数，对应 DrawTexturePro 的各实参
    struct Item {
        Texture2D texture;
        Rectangle source;
        Rectangle dest;
        Vector2 origin;
        float rotation;
        Color tint;
    };

    // layer 越大越后绘制（显示在上层）；imagepath 仅作调试标识，不参与排序
    static void push(Texture2D texture,
                     Rectangle source,
                     Rectangle dest,
                     Vector2 origin,
                     float rotation,
                     Color tint,
                     int layer,
                     std::string imagepath = "") {
        auto& self = instance();
        self.vec.emplace_back(
            Item{texture, source, dest, origin, rotation, tint},
            std::make_pair(layer, imagepath));
    }

    // 按 layer 排序后依次绘制，并清空本帧缓存
    static void flush() {
        auto& self = instance();
        std::sort(self.vec.begin(), self.vec.end(), [](auto& a, auto& b) {
            return a.second.first < b.second.first;
        });
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
    // pair: (绘制参数, (layer, 调试用路径))
    std::vector<std::pair<Item, std::pair<int, std::string>>> vec;
};

// 场景级渲染对象容器：统一管理 update / draw 生命周期。
// 使用延迟删除（deleted 标记），避免在遍历过程中直接 erase 迭代器失效。
class DrawList {
public:
    struct DrawEntry {
        BasicRenderClass* item = nullptr;
        bool deleted = false;  // true 时在下一次 update/draw 中物理移除
    };
    using iterator = std::list<DrawEntry>::iterator;

    // 更新所有存活对象；顺带清理已标记删除的条目
    void update() {
        for (auto it = draw_entries_.begin(); it != draw_entries_.end();) {
            if (it->deleted) {
                it = draw_entries_.erase(it);
                continue;
            }
            if (it->item != nullptr) {
                it->item->update();
            }
            ++it;
        }
    }
    void push_back(BasicRenderClass* item) {
        draw_entries_.push_back(DrawEntry{item, false});
    }
    // 逻辑删除：仅打标记，实际移除发生在 update/draw
    void erase(iterator it) {
        if (it == draw_entries_.end())
            return;
        it->deleted = true;
    }
    void clear() { draw_entries_.clear(); }

    // 先绘制各对象（内部会 push 到 DrawLayer），再 flush 统一按层输出
    void draw() {
        for (auto it = draw_entries_.begin(); it != draw_entries_.end();) {
            if (it->deleted) {
                it = draw_entries_.erase(it);
                continue;
            }
            if (it->item != nullptr) {
                it->item->draw();
            }
            ++it;
        }
        DrawLayer::flush();
    }
    // 预留的纹理绘制参数打包结构（供后续扩展使用）
    struct texturearg {
        Texture2D texture;
        Rectangle source, dest;
        Vector2 origin;
        float rotation;
        Color tint;
    };

private:
    std::list<DrawEntry> draw_entries_;
};
