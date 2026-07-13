#pragma once

#include "raylib.h"
#include "render/render.h"
#include <string>
#include<iostream>
using std::cerr;

// 基于 raylib 的简单 Sprite
// Image 在构造时载入；Texture2D 仅在 update 检测到新帧时更新；析构时 Unload
class Sprite : public Basic_Render_Class
{
public:
    // Load Image from file;
    explicit Sprite(const std::string& path):imagepath(path){}

    ~Sprite()
    {
        unload();
    }

    // Move-only: 防止拷贝导致的重复释放 / 泄漏
    Sprite(Sprite&& other) noexcept
        : imagepath(std::move(other.imagepath))
        , image(other.image)
        , texture(other.texture)
        , texture_frame(other.texture_frame)
        , vframes(other.vframes)
        , hframes(other.hframes)
        , frame(other.frame)
        , frame_process(other.frame_process)
        , during_time(other.during_time)
        , stopped(other.stopped)
        , flip_h(other.flip_h)
        , flip_v(other.flip_v)
        , hide(other.hide)
        , pos(other.pos)
        , offset(other.offset)
        , scale(other.scale)
    {
        other.texture = {};
        other.image = {};
        other.texture_frame = -1;
    }

    Sprite& operator=(Sprite&& other) noexcept {
        if (this == &other) return *this;
        unload();
        imagepath      = std::move(other.imagepath);
        image          = other.image;
        texture        = other.texture;
        texture_frame  = other.texture_frame;
        vframes        = other.vframes;
        hframes        = other.hframes;
        frame          = other.frame;
        frame_process  = other.frame_process;
        during_time    = other.during_time;
        stopped        = other.stopped;
        flip_h         = other.flip_h;
        flip_v         = other.flip_v;
        hide           = other.hide;
        pos            = other.pos;
        offset         = other.offset;
        scale          = other.scale;
        other.texture = {};
        other.image = {};
        other.texture_frame = -1;
        return *this;
    }

    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;

    // ---------- Position ----------
    Vector2 get_pos() const
    {
        return pos;
    }
    void set_pos(Vector2 p)
    {
        pos = p;
    }
    void set_pos(float x,float y)
    {
        set_pos({x, y});
    }

    bool get_hide() const
    {
        return hide;
    }
    void set_hide(bool h)
    {
        hide = h;
    }

    // Actual draw position = pos + offset
    Vector2 get_offset() const
    {
        return offset;
    }
    void set_offset(Vector2 o)
    {
        offset = o;
    }

    // Scale, (1,1) is original size
    Vector2 get_scale() const
    {
        return scale;
    }
    void set_scale(Vector2 s)
    {
        scale = s;
    }

    // ---------- Flip ----------
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

    // ---------- Sprite sheet (simple row/column frame slicing) ----------
    // hframes: columns; vframes: rows
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
            texture_frame = -1;  // force texture rebuild on next update
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

    // Current frame: numbered left-to-right, top-to-bottom (actual texture swap happens in update)
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

    // Frame coordinates (column, row)
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

    // ---------- Animation ----------
    // during_time: seconds to complete one full cycle; <=0 means no auto-play
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

    // Advance animation; rebuild texture if entering a new frame
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

        // Only update texture when frame changes (including first time)
    }

    // ---------- Draw ----------
    void draw()
    {
        if (hide) return;
        if (texture_frame != frame) {
            refresh_texture();
            texture_frame = frame;
        }

        Rectangle src{
            0.0f,
            0.0f,
            (float)texture.width,
            (float)texture.height,
        };

        // raylib: negative source width/height indicates flip
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
    void unload() {
        if (texture.id != 0) {
            UnloadTexture(texture);
            texture = {};
        }
    }

    std::string imagepath;
    Image image{};            // Sprite sheet source image, loaded at construction
    Texture2D texture{};      // Current frame texture, updated only in update()
    int texture_frame = -1;   // Frame index matching current texture; -1 = not yet generated

    int vframes = 1;  // Texture rows
    int hframes = 1;  // Texture columns
    int frame = 0;    // Logical current frame

    float frame_process = 0.0f;  // 当前帧已播放时间
    float during_time = 0.0f;    // 一整轮动画时长（秒）
    bool stopped = false;         // 是否暂停

    bool flip_h = false;  // Horizontal flip
    bool flip_v = false;  // Vertical flip

    bool hide = false; // Whether hidden (not rendered)

    Vector2 pos{0, 0};     // Scene coordinates
    Vector2 offset{0, 0};  // Draw offset, actual position = pos + offset
    Vector2 scale{1, 1};   // Scale

    // Crop one frame from image according to current frame index, rebuild texture
    void refresh_texture()
    {
        image = LoadImage(imagepath.c_str());
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
            UnloadImage(image);
            return;
        }

        float fw = (float)image.width / (float)hframes;
        float fh = (float)image.height / (float)vframes;
        int col = frame % hframes;
        int row = frame / hframes;
        Rectangle crop{col * fw, row * fh, fw, fh};

        // std::cout << "[DEBUG] crop = {" << crop.x << ", " << crop.y << ", " << crop.width << ", " << crop.height << "}" << std::endl;

        Image frame_img = ImageFromImage(image, crop);
        texture = LoadTextureFromImage(frame_img);
        UnloadImage(frame_img);
        UnloadImage(image);
    }
};
