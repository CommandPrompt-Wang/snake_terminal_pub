#pragma once
#include "raylib.h"
#include "render/sprite.h"
#include "render/render.h"
#include "game/snake.h"
#include "game/snake_state/animate_manager.h"
#include "config/config.h"
#include <string>
#include <array>
#include <vector>
#include <map>

constexpr float eps = 1e-6;

inline Vector2 operator + (const Vector2& a,const Vector2& b)
{
    return {a.x + b.x,a.y + b.y};
}
inline bool operator == (const Vector2& a,const Vector2& b)
{
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps;
}

// 在环形空间中检查 (center + offset) 是否等于 candidate（考虑边界卷绕）
inline bool is_toroidal_neighbor(const Vector2& center, const Vector2& candidate,
                                 int gridW, int gridH, float dx, float dy) {
    int cx = (int)center.x, cy = (int)center.y;
    int nx = (int)candidate.x, ny = (int)candidate.y;
    int ex = (cx + (int)dx + gridW) % gridW;
    int ey = (cy + (int)dy + gridH) % gridH;
    return nx == ex && ny == ey;
}

class SnakeBlock : public BasicRenderClass
{
public:
    SnakeBlock (int playerid = 0, Vector2 pos = {0,0})
    :
    playerid(playerid), pos(pos),
    side{Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2),Sprite("resources/up_side.png",2),Sprite("resources/right_side.png",2)},
    speedup{Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3),Sprite("resources/up_speed_side.png",3),Sprite("resources/right_speed_side.png",3)},
    fill(Sprite("resources/player" + std::to_string(playerid) + "fill.png",1)) {
        side[2].set_flip_v(1);speedup[2].set_flip_v(1);
        side[3].set_flip_h(1);speedup[3].set_flip_h(1);
    }
    ~SnakeBlock (){}

    // Move-only (Sprite 是 move-only 的)
    SnakeBlock(SnakeBlock&&) = default;
    SnakeBlock& operator=(SnakeBlock&&) = default;
    SnakeBlock(const SnakeBlock&) = delete;
    SnakeBlock& operator=(const SnakeBlock&) = delete;

    void set_status (SnakeBlock *pre, SnakeBlock *nxt, bool speed_up = false, bool toroidal = false) {
        side_status[0] = side_status[1] = side_status[2] = side_status[3] = 0;
        fill.set_hide(0);this->speed_up = speed_up;

        auto check_neighbor = [&](const SnakeBlock *neighbor, int i) {
            if (toroidal)
                return is_toroidal_neighbor(pos, neighbor->pos, GRID_W, GRID_H, mv[i].x, mv[i].y);
            else
                return neighbor->pos == pos + mv[i];
        };

        if(pre != nullptr) for (int i = 0; i < 4; i++) {
            side_status[i] |= check_neighbor(pre, i);
        }

        if (nxt != nullptr) {
            for (int i = 0; i < 4; i++)
                side_status[i] |= check_neighbor(nxt, i);
        }
    }
    void set_hide(bool hide){is_hide = hide;}
    void set_alpha(uint8_t a) {
        for (int i = 0; i < 4; i++) {
            side[i].set_alpha(a);
            speedup[i].set_alpha(a);
        }
        fill.set_alpha(a);
    }
    const int get_status() {
        int sta = 0;
        for(int i = 0;i < 4;i++)sta = (sta * 10) + (side_status[i]);
        return sta;
    }
    const Vector2 get_pos(){return pos;}
    void set_scale(Vector2 scale) {
        this->scale = scale;
        side[0].set_scale(scale);side[1].set_scale(scale);side[2].set_scale(scale);side[3].set_scale(scale);
        speedup[0].set_scale(scale);speedup[1].set_scale(scale);speedup[2].set_scale(scale);speedup[3].set_scale(scale);
        fill.set_scale(scale);
    }

    void set_pos (Vector2 pos) {
        this->pos = pos;
        Vector2 draw_position = Vector2{32 * pos.x, 32 * pos.y} + Vector2{0, 200};
        side[0].set_pos(draw_position);side[1].set_pos(draw_position);side[2].set_pos(draw_position);side[3].set_pos(draw_position);
        speedup[0].set_pos(draw_position);speedup[1].set_pos(draw_position);speedup[2].set_pos(draw_position);speedup[3].set_pos(draw_position);
        fill.set_pos(draw_position);
    }
    void set_pos (int x, int y) {
        set_pos (Vector2{(float)x, (float)y});
    }

    void draw () {
        if (is_hide == true)return;
        side[0].draw();side[1].draw();side[2].draw();side[3].draw();
        speedup[0].draw();speedup[1].draw();speedup[2].draw();speedup[3].draw();
        fill.draw();
    }
    void update () {
        for (int i = 0; i < 4; i++) {
            if (side_status[i] == 0) {
                if (speed_up) {
                    speedup[i].set_hide(0);side[i].set_hide(1);
                } else {
                    side[i].set_hide(0);speedup[i].set_hide(1);
                }
            } else {
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
    bool is_hide = false;
    inline static constexpr Vector2 mv[4] =
    {
        { 0,-1},
        { 1, 0},
        { 0, 1},
        {-1, 0}
    };
};
