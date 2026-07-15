#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include "backend/audioloader.h"

class AudioStreamPlayer {
    bool is_playing_ = false;
    bool loaded_successfully_ = false;
    double pitch_ = 1.0;
    double volume_db_ = 0.0;
    double volume_linear_ = 1.0;
    std::unique_ptr<AudioLoader> audio_loader_;

public:
    AudioStreamPlayer() = default;
    ~AudioStreamPlayer() = default;

    AudioStreamPlayer(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer& operator=(const AudioStreamPlayer&) = delete;
    AudioStreamPlayer(AudioStreamPlayer&&) noexcept = default;
    AudioStreamPlayer& operator=(AudioStreamPlayer&&) noexcept = default;

    bool load(const std::string& filepath) {
        audio_loader_ = std::make_unique<AudioLoader>(filepath);
        loaded_successfully_ =
            audio_loader_ &&
            audio_loader_->getLoadStatus() == AudioLoader::LoadStatus::Success;

        if (loaded_successfully_) {
            audio_loader_->pitch() = pitch_;
            audio_loader_->volume_linear() = volume_linear_;
            volume_db_ = 20.0 * std::log10((std::max)(volume_linear_, 1e-6));
            is_playing_ = false;
        }

        return loaded_successfully_;
    }

    void play() {
        if (!audio_loader_ || !loaded_successfully_) {
            return;
        }

        audio_loader_->play();
        is_playing_ = (audio_loader_->getPlayStatus() ==
                       AudioLoader::PlayStatus::Playing);
    }

    void resume() {
        if (!audio_loader_ || !loaded_successfully_) {
            return;
        }

        audio_loader_->resume();
        is_playing_ = (audio_loader_->getPlayStatus() ==
                       AudioLoader::PlayStatus::Playing);
    }

    void pause() {
        if (!audio_loader_ || !loaded_successfully_) {
            return;
        }

        audio_loader_->pause();
        is_playing_ = false;
    }

    void stop() {
        if (!audio_loader_ || !loaded_successfully_) {
            return;
        }

        audio_loader_->stop();
        is_playing_ = false;
    }

    const double pitch() const { return pitch_; }
    double& pitch() { return pitch_; }

    const double volume_db() const { return volume_db_; }
    double& volume_db() { return volume_db_; }

    const double volume_linear() const { return volume_linear_; }
    double& volume_linear() { return volume_linear_; }

    bool isPlaying() const { return is_playing_; }
    bool isLoadedSuccessfully() const { return loaded_successfully_; }
};