#pragma once

#include "miniaudio.h"
#include <atomic>
#include <string>
#include <vector>

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
        Stopped
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
    float volume_linear() const;
    void set_volume_linear(float v);

    /// 从同一文件重建实例
    AudioLoader clone() const;

    /// 混音到输出缓冲区（AudioManager 回调中调用），返回 false 表示已播完
    bool mix(float* out, ma_uint32 frameCount, ma_uint32 outChannels, ma_uint32 outSampleRate);

    ma_uint32 channels() const { return channels_; }
    ma_uint32 sample_rate() const { return sample_rate_; }

private:
    LoadStatus load_status_;
    std::atomic<PlayStatus> play_status_;
    double pitch_ = 1.0;
    std::atomic<float> volume_{1.0f};

    // 预解码缓冲区
    std::vector<float> pcm_buffer_;
    std::atomic<ma_uint64> cursor_{0};
    ma_uint64 total_frames_ = 0;
    ma_uint32 channels_ = 0;
    ma_uint32 sample_rate_ = 0;

    std::string filepath_;
};