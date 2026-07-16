#pragma once
#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "audio/audiostreamplayer.h"
#include "miniaudio.h"
#include "utils/log.h"

class AudioManager {
private:
    struct Command {
        enum Type { PlaySound, StopSound, AddSfx, SetVolume };
        Type type;
        std::string name;
        std::unique_ptr<AudioStreamPlayer> sfx;
        float volume = 1.0f;
    };

    std::map<std::string, AudioStreamPlayer> mp;
    std::string current_sound_;

    // 仅音频回调线程读写（命令队列 drain 后）
    std::vector<std::unique_ptr<AudioStreamPlayer>> sfx_pool_;

    std::mutex cmd_mutex_;
    std::vector<Command> cmd_queue_;

    ma_device device_{};
    bool device_initialized_ = false;

    void push_command(Command cmd);
    void process_commands();
    void cleanup_pool();

    static void ma_data_callback(ma_device* pDevice, void* pOutput,
                                   const void* pInput, ma_uint32 frameCount);

public:
    AudioManager() = default;
    ~AudioManager() {
        if (device_initialized_) {
            ma_device_stop(&device_);
            ma_device_uninit(&device_);
        }
        mp.clear();
    }

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool init_device() {
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

    void play_sound(const std::string& name) {
        if (mp.find(name) == mp.end()) { logw("sound not found: " + name); return; }
        Command cmd;
        cmd.type = Command::PlaySound;
        cmd.name = name;
        push_command(std::move(cmd));
    }

    void stop_sound() {
        Command cmd;
        cmd.type = Command::StopSound;
        push_command(std::move(cmd));
    }

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
};
