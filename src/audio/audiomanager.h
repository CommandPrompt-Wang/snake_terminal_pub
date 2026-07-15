#pragma once
#include <map>
#include <string>
#include "audio/audiostreamplayer.h"

class AudioManager {
private:
    std::map<std::string, AudioStreamPlayer> mp;
    std::string current_sound_;

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

    /// 独占播放（同一时间只有一个 sound）
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

    /// 并发播放（每个 sfx 独立，允许叠加）
    void play_sfx(const std::string& name) {
        if (auto* p = (*this)[name]) p->play();
    }

    bool erase_player(const std::string& name) {
        if (!has_player(name)) return false;
        mp.erase(name);
        return true;
    }

    bool has_player(const std::string& name) {
        return mp.find(name) != mp.end();
    }
};
