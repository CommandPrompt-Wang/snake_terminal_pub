#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "audio/audiostreamplayer.h"
#include "miniaudio.h"
#include "utils/log.h"

class AudioManager {
private:
    std::map<std::string, AudioStreamPlayer> mp;
    std::string current_sound_;

    // sfx 并发池
    std::map<std::string, std::vector<std::unique_ptr<AudioStreamPlayer>>> sfx_pool_;

    // 共享设备
    ma_device device_;
    bool device_initialized_ = false;

    void cleanup_pool(const std::string& name) {
        auto& v = sfx_pool_[name];
        v.erase(std::remove_if(v.begin(), v.end(),
            [](auto& p) { return !p->isPlaying(); }), v.end());
    }

    static void ma_data_callback(ma_device* pDevice, void* pOutput,
                                  const void* /*pInput*/, ma_uint32 frameCount) {
        auto* mgr = static_cast<AudioManager*>(pDevice->pUserData);
        auto* out = static_cast<float*>(pOutput);
        ma_uint32 ch = pDevice->playback.channels;
        ma_uint32 rate = pDevice->sampleRate;

        std::memset(out, 0, frameCount * ch * sizeof(float));

        // 混音当前 sound
        if (!mgr->current_sound_.empty()) {
            auto it = mgr->mp.find(mgr->current_sound_);
            if (it != mgr->mp.end()) {
                auto* ld = it->second.get_loader();
                if (ld) ld->mix(out, frameCount, ch, rate);
            }
        }

        // 混音 sfx 池
        for (auto& [name, pool] : mgr->sfx_pool_) {
            for (auto& p : pool) {
                auto* ld = p->get_loader();
                if (ld) ld->mix(out, frameCount, ch, rate);
            }
        }
    }

public:
    AudioManager() { std::memset(&device_, 0, sizeof(device_)); }
    ~AudioManager() {
        if (device_initialized_) {
            ma_device_stop(&device_);
            ma_device_uninit(&device_);
        }
        mp.clear();
    }

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
        auto it = mp.find(name);
        if (it == mp.end()) { logw("sound not found: " + name); return; }
        stop_sound();
        it->second.play();
        current_sound_ = name;
    }

    void stop_sound() {
        if (current_sound_.empty()) return;
        auto it = mp.find(current_sound_);
        if (it != mp.end()) it->second.stop();
        current_sound_.clear();
    }

    void play_sfx(const std::string& name) {
        auto it = mp.find(name);
        if (it == mp.end()) { logw("sfx not found: " + name); return; }
        cleanup_pool(name);
        auto inst = std::make_unique<AudioStreamPlayer>(it->second.clone());
        if (!inst->isLoadedSuccessfully()) { logw("sfx clone load failed: " + name); return; }
        inst->play();
        sfx_pool_[name].push_back(std::move(inst));
    }

    void set_volume_all(int vol_0_100) {
        float v = vol_0_100 / 100.0f;
        for (auto& [_, p] : mp) p.set_volume_linear(v);
    }

    bool has_player(const std::string& name) {
        return mp.find(name) != mp.end();
    }
};
