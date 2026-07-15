#pragma once
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "audio/audiostreamplayer.h"

class AudioManager {
private:
    std::map<std::string, AudioStreamPlayer> mp;
    std::string current_sound_;

    // sfx 并发池
    std::map<std::string, std::vector<std::unique_ptr<AudioStreamPlayer>>> sfx_pool_;

    void cleanup_pool(const std::string& name) {
        auto& v = sfx_pool_[name];
        v.erase(std::remove_if(v.begin(), v.end(),
            [](auto& p) { return !p->isPlaying(); }), v.end());
    }

public:
    AudioManager() {}
    ~AudioManager() { mp.clear(); }

    void add_player(const std::string& name, AudioStreamPlayer& p) {
        mp[name] = std::move(p);
    }

    AudioStreamPlayer* operator[](std::string name) {
        auto it = mp.find(name);
        return it != mp.end() ? &it->second : nullptr;
    }

    void play_sound(const std::string& name) {
        if (auto* p = (*this)[name]) {
            stop_sound();
            p->play();
            current_sound_ = name;
        }
    }

    void stop_sound() {
        if (!current_sound_.empty()) {
            if (auto* p = (*this)[current_sound_]) p->stop();
            current_sound_.clear();
        }
    }

    void play_sfx(const std::string& name) {
        auto* src = (*this)[name];
        if (!src) return;
        cleanup_pool(name);
        auto inst = std::make_unique<AudioStreamPlayer>(src->clone());
        if (!inst->isLoadedSuccessfully()) return;
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
