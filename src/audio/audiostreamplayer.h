#pragma once

#include "backend/audioloader.h"
#include <memory>
#include <string>

class AudioStreamPlayer {
    bool is_playing=false;
    bool loaded_successfully=false;
    double pitch=1.0;
    double volume_db=0.0;
    double volume_linear=1.0;
    std::unique_ptr<AudioLoader> audio_loader;

public:
    AudioStreamPlayer();
    ~AudioStreamPlayer();
    AudioStreamPlayer(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer& operator=(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer(AudioStreamPlayer&&);
    AudioStreamPlayer& operator=(AudioStreamPlayer&&);

    bool load(const std::string& filepath);
    void play();
    void pause();
    void stop();
    const double pitch() const;
    double& pitch();
    const double volume_db() const;
    double& volume_db();
    const double volume_linear() const;
    double& volume_linear();

    bool isPlaying() const;
    bool isLoadedSuccessfully() const;
};