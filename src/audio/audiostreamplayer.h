#pragma once

#include "backend/audioloader.h"
#include <cmath>
#include <memory>
#include <string>
#include <utility>

class AudioStreamPlayer {
    bool is_playing = false;
    bool loaded_successfully = false;
    double pitch = 1.0;
    double volume_db = 0.0;
    double volume_linear = 1.0;
    std::unique_ptr<AudioLoader> audio_loader;

public:
    AudioStreamPlayer() = default;
    ~AudioStreamPlayer() = default;

    AudioStreamPlayer(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer& operator=(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer(AudioStreamPlayer&&) noexcept = default;
    AudioStreamPlayer& operator=(AudioStreamPlayer&&) noexcept = default;

    bool load(const std::string& filepath)
    {
        audio_loader = std::make_unique<AudioLoader>(filepath);
        loaded_successfully = audio_loader && audio_loader->getLoadStatus() == AudioLoader::LoadStatus::Success;

        if (loaded_successfully)
        {
            audio_loader->pitch() = pitch;
            audio_loader->volume_linear() = volume_linear;
            volume_db = 20.0 * std::log10(std::max(volume_linear, 1e-6));
            is_playing = false;
        }

        return loaded_successfully;
    }

    void play()
    {
        if (!audio_loader || !loaded_successfully)
        {
            return;
        }

        audio_loader->play();
        is_playing = (audio_loader->getPlayStatus() == AudioLoader::PlayStatus::Playing);
    }

    void pause()
    {
        if (!audio_loader || !loaded_successfully)
        {
            return;
        }

        audio_loader->pause();
        is_playing = false;
    }

    void stop()
    {
        if (!audio_loader || !loaded_successfully)
        {
            return;
        }

        audio_loader->stop();
        is_playing = false;
    }

    const double pitch() const { return pitch; }
    double& pitch() { return pitch; }

    const double volume_db() const { return volume_db; }
    double& volume_db() { return volume_db; }

    const double volume_linear() const { return volume_linear; }
    double& volume_linear() { return volume_linear; }

    bool isPlaying() const { return is_playing; }
    bool isLoadedSuccessfully() const { return loaded_successfully; }
};