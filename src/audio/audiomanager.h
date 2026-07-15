#pragma once
#include "audio/audiostreamplayer.h"
#include "audio/audiostreamplayer.h"
#include <map>
#include <string>

class AudioManager
{
private:
    std::map<std::string,AudioStreamPlayer> mp;
public:
    AudioManager (){}
    ~AudioManager ()
    {
        mp.clear();
    }
    void add_player (const std::string& name,AudioStreamPlayer& audiostreamplayer)
    {
        mp[name] = std::move(audiostreamplayer);
    }
    AudioStreamPlayer* operator[] (std::string name)
    {
        if (mp.find(name) == mp.end())
        {
            return nullptr;
        }
        return &mp[name];
    }
    bool erase_player(const std::string& name)
    {
        if (mp.find(name) == mp.end())
        {
            return false;
        }
        mp.erase(name);
        return true;
    }
};
