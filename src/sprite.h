#pragma once

#include "raylib.h"

// 基于 raylib 的简单 Sprite
// 支持：整图绘制、按行列切帧、翻转、偏移、简单逐帧动画
class Sprite
{
public:
    Sprite() = default;

    // 从文件加载纹理
    explicit Sprite(const char* path)
    {
        texture = LoadTexture(path);
        owns_texture = (texture.id != 0);
    }

    // 从 Image 创建纹理（调用方可自行 UnloadImage）
    explicit Sprite(Image src)
    {
        texture = LoadTextureFromImage(src);
        owns_texture = (texture.id != 0);
    }

    // 使用已有 Texture2D（不负责卸载）
    explicit Sprite(Texture2D tex) : texture(tex)
    {
    }

    ~Sprite()
    {
        if (owns_texture && texture.id != 0)
        {
            UnloadTexture(texture);
        }
    }

    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;

    // ---------- 纹理 ----------
    Texture2D get_texture() const
    {
        return texture;
    }

    void set_texture(Texture2D tex)
    {
        if (owns_texture && texture.id != 0)
        {
            UnloadTexture(texture);
        }
        texture = tex;
        owns_texture = false;
    }

    // ---------- 位置 ----------
    Vector2 get_pos() const
    {
        return pos;
    }
    void set_pos(Vector2 p)
    {
        pos = p;
    }

    // 实际绘制位置 = pos + offset
    Vector2 get_offset() const
    {
        return offset;
    }
    void set_offset(Vector2 o)
    {
        offset = o;
    }

    // 缩放，(1,1) 为原始大小
    Vector2 get_scale() const
    {
        return scale;
    }
    void set_scale(Vector2 s)
    {
        scale = s;
    }

    // ---------- 翻转 ----------
    bool is_flipped_h() const
    {
        return flip_h;
    }
    void set_flip_h(bool f)
    {
        flip_h = f;
    }

    bool is_flipped_v() const
    {
        return flip_v;
    }
    void set_flip_v(bool f)
    {
        flip_v = f;
    }

    // ---------- 精灵表（简单行列切帧） ----------
    // hframes：列数；vframes：行数
    int get_hframes() const
    {
        return hframes;
    }
    void set_hframes(int n)
    {
        if (n >= 1)
        {
            hframes = n;
            if (frame >= hframes * vframes)
            {
                frame = 0;
            }
        }
    }

    int get_vframes() const
    {
        return vframes;
    }
    void set_vframes(int n)
    {
        if (n >= 1)
        {
            vframes = n;
            if (frame >= hframes * vframes)
            {
                frame = 0;
            }
        }
    }

    // 当前帧：从左到右、从上到下编号
    int get_frame() const
    {
        return frame;
    }
    void set_frame(int f)
    {
        if (f >= 0 && f < hframes * vframes)
        {
            frame = f;
        }
    }

    // 帧坐标 (列, 行)
    Vector2 get_frame_coords() const
    {
        return Vector2{
            (float)(frame % hframes),
            (float)(frame / hframes),
        };
    }

    void set_frame_coords(Vector2 coords)
    {
        int x = (int)coords.x;
        int y = (int)coords.y;
        if (x >= 0 && x < hframes && y >= 0 && y < vframes)
        {
            frame = y * hframes + x;
        }
    }

    // ---------- 动画 ----------
    // during_time：播完一整轮所需秒数；<=0 不自动播放
    float get_during_time() const
    {
        return during_time;
    }
    void set_during_time(float seconds)
    {
        during_time = seconds;
    }

    void play()
    {
        stopped = false;
    }
    void stop()
    {
        stopped = true;
    }
    bool is_stopped() const
    {
        return stopped;
    }

    void reset_animation()
    {
        frame = 0;
        frame_process = 0.0f;
    }

    // 每帧调用；用 GetFrameTime 推进动画
    void update()
    {
        if (stopped || during_time <= 0.0f)
        {
            return;
        }
        int total = hframes * vframes;
        if (total <= 1)
        {
            return;
        }

        frame_process += GetFrameTime();
        float frame_duration = during_time / (float)total;
        while (frame_process >= frame_duration)
        {
            frame_process -= frame_duration;
            frame = (frame + 1) % total;
        }
    }

    // ---------- 绘制 ----------
    // 当前帧在纹理中的源矩形
    Rectangle get_source_rect() const
    {
        float fw = (float)texture.width / (float)hframes;
        float fh = (float)texture.height / (float)vframes;
        int col = frame % hframes;
        int row = frame / hframes;
        return Rectangle{col * fw, row * fh, fw, fh};
    }

    void draw() const
    {
        if (texture.id == 0)
        {
            return;
        }

        Rectangle src = get_source_rect();

        // raylib：source 宽/高为负表示翻转
        if (flip_h)
        {
            src.x += src.width;
            src.width = -src.width;
        }
        if (flip_v)
        {
            src.y += src.height;
            src.height = -src.height;
        }

        float fw = src.width < 0 ? -src.width : src.width;
        float fh = src.height < 0 ? -src.height : src.height;

        Rectangle dest{
            pos.x + offset.x,
            pos.y + offset.y,
            fw * scale.x,
            fh * scale.y,
        };

        DrawTexturePro(texture, src, dest, Vector2{0, 0}, 0.0f, WHITE);
    }

private:
    Texture2D texture{};   // 当前纹理
    bool owns_texture = false;

    int vframes = 1;  // 纹理行数
    int hframes = 1;  // 纹理列数
    int frame = 0;    // 当前帧

    float frame_process = 0.0f;  // 当前帧已播放时间
    float during_time = 0.0f;    // 一整轮动画时长（秒）
    bool stopped = true;         // 是否暂停

    bool flip_h = false;  // 水平翻转
    bool flip_v = false;  // 垂直翻转

    Vector2 pos{0, 0};     // 场景坐标
    Vector2 offset{0, 0};  // 绘制偏移，实际位置 = pos + offset
    Vector2 scale{1, 1};   // 缩放
};
