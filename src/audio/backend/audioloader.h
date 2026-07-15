#pragma once

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#include <string>

class AudioLoader {
public:
    AudioLoader(std::string filepath);
    ~AudioLoader();

    enum class LoadStatus {
        Success=0,
        FailureWhenInitializing,
        FailureWhenReading,
        FailureWhenPlaying,
        FailureWhenDestroying
    };
    enum class PlayStatus {
        Error=-1,
        Playing=0,
        Paused,
        Stopped
    };

    void play();
    void pause();
    void stop();
    double getCurrentTime() const;
    
    PlayStatus getPlayStatus() const;
    LoadStatus getLoadStatus() const;
    const double pitch() const;
    double& pitch();
    const double volume_linear() const;
    double& volume_linear();

private:
    drmp3 audio_data;
    LoadStatus load_status;
    PlayStatus play_status;
};