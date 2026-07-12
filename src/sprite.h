#pragma once

#include "raylib.h"
#include "render.h"
#include <string>

// 基于 raylib 的简单 Sprite
// Image 在构造时载入；Texture2D 仅在 update 检测到新帧时更新；析构时 Unload
class Sprite : Basic_Render_Class
{
public:
    // 从文件加载 Image；
    explicit Sprite(const std::string& path)
    {
        image = LoadImage(path.c_str());
    }

    // 拷贝 Image；调用方可自行 UnloadImage
    explicit Sprite(Image src = {})
    {
        if (src.data != nullptr)
        {
            image = ImageCopy(src);
        }
    }

    ~Sprite()
    {
        if (texture.id != 0)
        {
            UnloadTexture(texture);
        }
        if (image.data != nullptr)
        {
            UnloadImage(image);
        }
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

    bool get_hide() const
    {
        return hide;
    }
    void set_hide(bool h)
    {
        hide = h;
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
            texture_frame = -1;  // 强制下次 update 重建纹理
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
            texture_frame = -1;
        }
    }

    // 当前帧：从左到右、从上到下编号（真正换贴图在 update 里做）
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

    // 推进动画；若进入新帧则重建 texture
    void update()
    {

        if (hframes * vframes != 1 && !stopped && during_time > 0.0f)
        {
            int total = hframes * vframes;
            if (total > 1)
            {
                frame_process += GetFrameTime();
                float frame_duration = during_time / (float)total;
                while (frame_process >= frame_duration)
                {
                    frame_process -= frame_duration;
                    frame = (frame + 1) % total;
                }
            }
        }

        // 仅在帧变化（含首次）时更新 texture
        if (frame != texture_frame)
        {
            refresh_texture();
            texture_frame = frame;
        }
    }

    // ---------- 绘制 ----------
    void draw()
    {
        if (hide) return;
        if (texture.id == 0)
        {
            return;
        }

        Rectangle src{
            0.0f,
            0.0f,
            (float)texture.width,
            (float)texture.height,
        };

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
    Image image{};            // 精灵表原图，构造时载入
    Texture2D texture{};      // 当前帧纹理，仅在 update 中更新
    int texture_frame = -1;   // 当前 texture 对应的帧；-1 表示尚未生成

    int vframes = 1;  // 纹理行数
    int hframes = 1;  // 纹理列数
    int frame = 0;    // 逻辑当前帧

    float frame_process = 0.0f;  // 当前帧已播放时间
    float during_time = 0.0f;    // 一整轮动画时长（秒）
    bool stopped = true;         // 是否暂停

    bool flip_h = false;  // 水平翻转
    bool flip_v = false;  // 垂直翻转

    bool hide = false; // 是否隐藏（不渲染）

    Vector2 pos{0, 0};     // 场景坐标
    Vector2 offset{0, 0};  // 绘制偏移，实际位置 = pos + offset
    Vector2 scale{1, 1};   // 缩放

    // 按当前 frame 从 image 裁出一帧，重建 texture
    void refresh_texture()
    {
        if (image.data == nullptr || hframes < 1 || vframes < 1)
        {
            return;
        }

        if (texture.id != 0)
        {
            UnloadTexture(texture);
            texture = {};
        }

        if (hframes == 1 && vframes == 1)
        {
            texture = LoadTextureFromImage(image);
            return;
        }

        float fw = (float)image.width / (float)hframes;
        float fh = (float)image.height / (float)vframes;
        int col = frame % hframes;
        int row = frame / hframes;
        Rectangle crop{col * fw, row * fh, fw, fh};

        Image frame_img = ImageFromImage(image, crop);
        texture = LoadTextureFromImage(frame_img);
        UnloadImage(frame_img);
    }
};
