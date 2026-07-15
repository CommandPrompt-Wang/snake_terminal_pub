#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include "backend/audioloader.h"
#include "utils/log.h"

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

        if (!loaded_successfully_) {
            logw("audio load failed: " + filepath);
            return false;
        }

        audio_loader_->pitch() = pitch_;
        audio_loader_->volume_linear() = volume_linear_;
        volume_db_ = 20.0 * std::log10((std::max)(volume_linear_, 1e-6));
        is_playing_ = false;
        return true;
    }

    void play() {
        if (!audio_loader_ || !loaded_successfully_) {
            if (!loaded_successfully_) logw("play() called on unloaded player");
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
    double& volume_db() {
        return volume_db_;
    }

    const double volume_linear() const { return volume_linear_; }
    double& volume_linear() {
        return volume_linear_;
    }

    void set_volume_db(double db) {
        volume_db_ = db;
        volume_linear_ = std::pow(10.0, db / 20.0);
        if (audio_loader_) audio_loader_->volume_linear() = volume_linear_;
    }

    void set_volume_linear(double v) {
        volume_linear_ = v;
        volume_db_ = 20.0 * std::log10(std::max(v, 1e-6));
        if (audio_loader_) audio_loader_->volume_linear() = v;
    }

    AudioStreamPlayer clone() const {
        AudioStreamPlayer p;
        if (audio_loader_) {
            p.audio_loader_ = std::make_unique<AudioLoader>(audio_loader_->clone());
            p.loaded_successfully_ = p.audio_loader_->getLoadStatus() == AudioLoader::LoadStatus::Success;
            if (!p.loaded_successfully_) logw("sfx clone failed: loader not in Success state");
        }
        p.pitch_ = pitch_;
        p.volume_linear_ = volume_linear_;
        p.volume_db_ = volume_db_;
        if (p.audio_loader_) p.audio_loader_->volume_linear() = volume_linear_;
        return p;
    }

    bool isPlaying() const {
        return audio_loader_ && audio_loader_->getPlayStatus() == AudioLoader::PlayStatus::Playing;
    }
    bool isLoadedSuccessfully() const { return loaded_successfully_; }

    AudioLoader* get_loader() { return audio_loader_.get(); }
    const AudioLoader* get_loader() const { return audio_loader_.get(); }
};