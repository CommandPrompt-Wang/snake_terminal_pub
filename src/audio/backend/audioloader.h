#pragma once

#include "miniaudio.h"
#include <atomic>
#include <string>
#include <vector>

// 音频解码与混音层：启动时将 MP3 全量解码到内存 PCM，运行时由 AudioManager 回调调用 mix() 输出。
// 不持有 ma_device；设备生命周期由 AudioManager 统一管理。
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

    /// 从同一文件重新解码，供 play_sfx 并发播放（各实例独立 cursor）
    AudioLoader clone() const;

    /// 混音到输出缓冲区（AudioManager 回调中调用），返回 false 表示已播完
    bool mix(float* out, ma_uint32 frameCount, ma_uint32 outChannels, ma_uint32 outSampleRate);

    ma_uint32 channels() const { return channels_; }
    ma_uint32 sample_rate() const { return sample_rate_; }

private:
    LoadStatus load_status_;
    std::atomic<PlayStatus> play_status_;  // 回调线程读写，主线程只通过 play/stop 修改
    double pitch_ = 1.0;                     // 预留，当前 mix 未应用
    std::atomic<float> volume_{1.0f};        // 线性音量 [0, 1]

    // 预解码 PCM（interleaved float32）；构造时一次性读入，避免实时解码
    std::vector<float> pcm_buffer_;
    std::atomic<ma_uint64> cursor_{0};       // 当前播放位置（帧索引）
    ma_uint64 total_frames_ = 0;
    ma_uint32 channels_ = 0;
    ma_uint32 sample_rate_ = 0;

    std::string filepath_;
};