#pragma once
#include <map>
#include <string>
#include "audio/audiostreamplayer.h"

class AudioManager {
private:
    std::map<std::string, AudioStreamPlayer> mp;

public:
    AudioManager() {}
    ~AudioManager() { mp.clear(); }
    void add_player(const std::string& name,
                    AudioStreamPlayer& audiostreamplayer) {
        mp[name] = std::move(audiostreamplayer);
    }

    AudioStreamPlayer* operator[](std::string name) {
        if (mp.find(name) == mp.end()) {
            return nullptr;
        }
        return &mp[name];
    }

    bool erase_player(const std::string& name) {
        if (! has_player(name)) {
            return false;
        }
        mp.erase(name);
        return true;
    }

    bool has_player(const std::string& name) {
        return mp.find(name) != mp.end();
    }
};
