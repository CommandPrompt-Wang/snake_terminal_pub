#pragma once
#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "audio/audiostreamplayer.h"
#include "miniaudio.h"
#include "utils/log.h"

// 音频管线顶层：全局唯一 miniaudio 播放设备 + 命名播放器注册表。
//
// 单设备策略（重要）：
//   整个进程只 init/start 一次 ma_device，仅在 AudioManager 析构时 stop/uninit。
//   切场景、停 BGM、播 SFX 均只操作 AudioLoader 的 play/stop，不碰设备本身。
//   原因：WSL/PulseAudio 等对频繁 ma_device_init/uninit 极不稳定，容易卡死或崩溃。
//   无音源时回调仍运行，输出 memset 静音帧，保持设备常开。
//
// 线程模型：
//   主线程  → push_command()  投递 PlaySound / StopSound / AddSfx / SetVolume
//   音频线程 → ma_data_callback  每帧 drain 命令队列 → 混音 → 输出
//
// 播放模式：
//   play_sound  独占背景音乐（同时只播一条，存于 mp + current_sound_）
//   play_sfx    一次性音效（clone 后加入 sfx_pool_，播完自动回收）
class AudioManager {
private:
    struct Command {
        enum Type { PlaySound, StopSound, AddSfx, SetVolume, PlayQuitSfx };
        Type type;
        std::string name;
        std::unique_ptr<AudioStreamPlayer> sfx;
        float volume = 1.0f;
    };

    std::map<std::string, AudioStreamPlayer> mp;  // 预注册的命名音源模板
    std::string current_sound_;                    // 当前正在播放的 BGM 名称

    // 仅音频回调线程读写（命令队列 drain 后）
    std::vector<std::unique_ptr<AudioStreamPlayer>> sfx_pool_;
    std::unique_ptr<AudioStreamPlayer> quit_sfx_;
    std::atomic<bool> quit_sfx_playing_{false};

    std::mutex cmd_mutex_;
    std::vector<Command> cmd_queue_;  // 主线程写入，回调线程 swap 后消费

    ma_device device_{};
    bool device_initialized_ = false;  // 进程内至多 init 一次，禁止重复开关

    void push_command(Command cmd);
    void process_commands();
    void cleanup_pool();  // 移除 sfx_pool_ 中已播完的实例

    static void ma_data_callback(ma_device* pDevice, void* pOutput,
                                   const void* pInput, ma_uint32 frameCount);

public:
    AudioManager() = default;
    ~AudioManager() {
        // 唯一允许 uninit 设备的时机；不要在场景切换时调用
        if (device_initialized_) {
            ma_device_stop(&device_);
            ma_device_uninit(&device_);
        }
        mp.clear();
    }

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // 进程启动时调用一次；已初始化则直接返回 true
    bool init_device() {
        if (device_initialized_) return true;

        // 固定 48kHz 立体声 float32 输出；所有源在 mix() 中重采样到此格式
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 2;
        config.sampleRate        = 48000;
        config.dataCallback      = ma_data_callback;
        config.pUserData         = this;
        if (ma_device_init(nullptr, &config, &device_) != MA_SUCCESS) {
            loge("audio device init failed");
            return false;
        }
        device_initialized_ = true;
        ma_device_start(&device_);
        log("audio device started: 48000Hz stereo f32");
        return true;
    }

    void add_player(const std::string& name, AudioStreamPlayer p) {
        mp[name] = std::move(p);
    }

    [[deprecated("use play_sfx / play_sound instead")]]
    AudioStreamPlayer* operator[](std::string name) {
        auto it = mp.find(name);
        return it != mp.end() ? &it->second : nullptr;
    }

    // 切换背景音乐：先停当前，再从头播放 name
    void play_sound(const std::string& name) {
        if (mp.find(name) == mp.end()) { logw("sound not found: " + name); return; }
        Command cmd;
        cmd.type = Command::PlaySound;
        cmd.name = name;
        push_command(std::move(cmd));
    }

    // 停止当前 BGM 播放；不停止 ma_device（设备保持常开）
    void stop_sound() {
        Command cmd;
        cmd.type = Command::StopSound;
        push_command(std::move(cmd));
    }

    // 克隆模板并加入 sfx_pool_，可与 BGM 及其他 SFX 叠加播放
    void play_sfx(const std::string& name) {
        auto it = mp.find(name);
        if (it == mp.end()) { logw("sfx not found: " + name); return; }
        auto inst = std::make_unique<AudioStreamPlayer>(it->second.clone());
        if (!inst->isLoadedSuccessfully()) { logw("sfx clone load failed: " + name); return; }
        Command cmd;
        cmd.type = Command::AddSfx;
        cmd.sfx = std::move(inst);
        push_command(std::move(cmd));
    }

    void set_volume_all(int vol_0_100) {
        Command cmd;
        cmd.type = Command::SetVolume;
        cmd.volume = vol_0_100 / 100.0f;
        push_command(std::move(cmd));
    }

    bool has_player(const std::string& name) {
        return mp.find(name) != mp.end();
    }

    void play_quit_sfx();
    bool is_quit_sfx_playing() const {
        return quit_sfx_playing_.load(std::memory_order_acquire);
    }
};
