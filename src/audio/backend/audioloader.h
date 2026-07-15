#pragma once

#include "dr_mp3.h"
#include "miniaudio.h"
#include <atomic>
#include <string>

class AudioLoader {
public:
    AudioLoader(std::string filepath);
    ~AudioLoader();
    AudioLoader(const AudioLoader&) = delete;
    AudioLoader& operator=(const AudioLoader&) = delete;
    AudioLoader(AudioLoader&&) noexcept;
    AudioLoader& operator=(AudioLoader&&) noexcept;

    enum class LoadStatus {
        Success = 0,
        FileNotFound,
        InvalidFormat,
        DecoderInitFailed,
        DeviceInitFailed,
        DeviceStartFailed,
        OutOfMemory,
        UnknownError
    };
    enum class PlayStatus {
        Error=-1,
        Playing=0,
        Paused,
        Stopped     // means the audio has finished playing
    };

    void play();
    void resume();
    void pause();
    void stop();
    double getDuration() const;
    double getCurrentTime() const;
    double setCurrentTime(double time);
    
    PlayStatus getPlayStatus() const;
    LoadStatus getLoadStatus() const;
    const double pitch() const;
    double& pitch();
    const double volume_linear() const;
    double& volume_linear();

private:
    drmp3 audio_data_;
    ma_device device_;
    bool device_initialized_ = false;
    LoadStatus load_status_;
    std::atomic<PlayStatus> play_status_;
    double pitch_ = 1.0;
    double volume_ = 1.0;

    static void ma_data_callback(ma_device* pDevice, void* pOutput,
                                  const void* pInput, ma_uint32 frameCount);
};