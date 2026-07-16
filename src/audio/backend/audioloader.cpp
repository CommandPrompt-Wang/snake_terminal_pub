// 单编译单元：先定义实现宏，再引入单头文件库的实现段
#define DR_MP3_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#include "dr_mp3.h"
#include "miniaudio.h"

#include "audioloader.h"
#include "utils/log.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

AudioLoader::AudioLoader(std::string filepath)
    : load_status_(LoadStatus::UnknownError)
    , play_status_(PlayStatus::Stopped)
    , filepath_(std::move(filepath))
{
    FILE* test = fopen(filepath_.c_str(), "rb");
    if (!test) { load_status_ = LoadStatus::FileNotFound; loge("audio file not found: " + filepath_); return; }
    fclose(test);

    drmp3 mp3;
    if (!drmp3_init_file(&mp3, filepath_.c_str(), nullptr)) {
        load_status_ = LoadStatus::DecoderInitFailed; loge("mp3 decoder init failed: " + filepath_); return;
    }

    channels_ = mp3.channels;
    sample_rate_ = mp3.sampleRate;
    total_frames_ = drmp3_get_pcm_frame_count(&mp3);

    pcm_buffer_.resize(total_frames_ * channels_);
    drmp3_read_pcm_frames_f32(&mp3, total_frames_, pcm_buffer_.data());
    drmp3_uninit(&mp3);

    load_status_ = LoadStatus::Success;
    logd("audio loaded: " + filepath_ + " " + std::to_string(sample_rate_) + "Hz "
         + std::to_string(channels_) + "ch " + std::to_string(total_frames_) + " frames");
}

AudioLoader::~AudioLoader() {
    // 无需清理 ma_device，由 AudioManager 管理
}

AudioLoader::AudioLoader(AudioLoader&& other) noexcept
    : load_status_(other.load_status_)
    , play_status_(other.play_status_.load(std::memory_order_acquire))
    , pcm_buffer_(std::move(other.pcm_buffer_))
    , total_frames_(other.total_frames_)
    , channels_(other.channels_)
    , sample_rate_(other.sample_rate_)
    , filepath_(std::move(other.filepath_))
{
    cursor_.store(other.cursor_.load(std::memory_order_acquire), std::memory_order_release);

    other.load_status_ = LoadStatus::UnknownError;
    other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
    other.total_frames_ = 0;
    other.cursor_.store(0, std::memory_order_release);
}

AudioLoader& AudioLoader::operator=(AudioLoader&& other) noexcept {
    if (this != &other) {
        play_status_.store(PlayStatus::Stopped, std::memory_order_release);

        load_status_ = other.load_status_;
        play_status_.store(other.play_status_.load(std::memory_order_acquire), std::memory_order_release);
        pcm_buffer_ = std::move(other.pcm_buffer_);
        total_frames_ = other.total_frames_;
        cursor_.store(other.cursor_.load(std::memory_order_acquire), std::memory_order_release);
        channels_ = other.channels_;
        sample_rate_ = other.sample_rate_;
        filepath_ = std::move(other.filepath_);

        other.load_status_ = LoadStatus::UnknownError;
        other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
        other.total_frames_ = 0;
        other.cursor_.store(0, std::memory_order_release);
    }
    return *this;
}

void AudioLoader::play() {
    if (load_status_ != LoadStatus::Success) return;
    cursor_.store(0, std::memory_order_release);
    play_status_.store(PlayStatus::Playing, std::memory_order_release);
}

void AudioLoader::resume() { play(); }

void AudioLoader::pause() { stop(); }

void AudioLoader::stop() {
    play_status_.store(PlayStatus::Stopped, std::memory_order_release);
    cursor_.store(0, std::memory_order_release);
}

bool AudioLoader::mix(float* out, ma_uint32 frameCount, ma_uint32 outChannels, ma_uint32 outSampleRate) {
    if (play_status_.load(std::memory_order_acquire) != PlayStatus::Playing)
        return false;

    ma_uint64 cur = cursor_.load(std::memory_order_acquire);
    float vol = volume_.load(std::memory_order_relaxed);
    float srcRatio = static_cast<float>(sample_rate_) / static_cast<float>(outSampleRate);

    // 同采样率 + 同声道 → 快速路径（无插值）
    if (srcRatio == 1.0f) {
        static bool logged_fast = false;
        if (!logged_fast) { logd("mix fast path (no resample): " + filepath_); logged_fast = true; }
        ma_uint64 remain = total_frames_ - cur;
        ma_uint64 n = frameCount;
        if (n > remain) n = remain;

        if (n == 0) {
            play_status_.store(PlayStatus::Stopped, std::memory_order_release);
            return false;
        }

        if (channels_ == outChannels) {
            for (ma_uint64 i = 0; i < n * channels_; ++i)
                out[i] += pcm_buffer_[cur * channels_ + i] * vol;
        } else if (channels_ == 1 && outChannels == 2) {
            for (ma_uint64 i = 0; i < n; ++i) {
                float s = pcm_buffer_[cur + i] * vol;
                out[i * 2] += s;
                out[i * 2 + 1] += s;
            }
        } else if (channels_ == 2 && outChannels == 1) {
            for (ma_uint64 i = 0; i < n; ++i) {
                float s = (pcm_buffer_[(cur + i) * 2] + pcm_buffer_[(cur + i) * 2 + 1]) * 0.5f * vol;
                out[i] += s;
            }
        }

        cur += n;
        cursor_.store(cur, std::memory_order_release);

        if (cur >= total_frames_) {
            play_status_.store(PlayStatus::Stopped, std::memory_order_release);
            return false;
        }
        return true;
    }

    // 采样率转换 + 线性插值
    static bool logged_interp = false;
    if (!logged_interp) {
        logd("mix resample path: " + filepath_ + " src=" + std::to_string(sample_rate_)
             + "Hz dst=" + std::to_string(outSampleRate) + "Hz");
        logged_interp = true;
    }
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        // 计算源位置
        float srcPos = static_cast<float>(cur) + static_cast<float>(i) * srcRatio;
        ma_uint64 srcIdx = static_cast<ma_uint64>(srcPos);
        float frac = srcPos - static_cast<float>(srcIdx);

        if (srcIdx >= total_frames_ - 1) {
            // 已播完
            play_status_.store(PlayStatus::Stopped, std::memory_order_release);
            return false;
        }

        if (channels_ == outChannels) {
            for (ma_uint32 c = 0; c < channels_; ++c) {
                float a = pcm_buffer_[srcIdx * channels_ + c];
                float b = pcm_buffer_[(srcIdx + 1) * channels_ + c];
                out[i * outChannels + c] += (a + (b - a) * frac) * vol;
            }
        } else if (channels_ == 1 && outChannels == 2) {
            float a = pcm_buffer_[srcIdx];
            float b = pcm_buffer_[srcIdx + 1];
            float s = (a + (b - a) * frac) * vol;
            out[i * 2] += s;
            out[i * 2 + 1] += s;
        } else if (channels_ == 2 && outChannels == 1) {
            float aL = pcm_buffer_[srcIdx * 2];
            float bL = pcm_buffer_[(srcIdx + 1) * 2];
            float aR = pcm_buffer_[srcIdx * 2 + 1];
            float bR = pcm_buffer_[(srcIdx + 1) * 2 + 1];
            out[i] += ((aL + aR) * 0.5f + ((bL + bR) * 0.5f - (aL + aR) * 0.5f) * frac) * vol;
        }
    }

    // 推进 cursor（源帧）
    cur += static_cast<ma_uint64>(static_cast<float>(frameCount) * srcRatio);
    cursor_.store(cur, std::memory_order_release);

    if (cur >= total_frames_) {
        play_status_.store(PlayStatus::Stopped, std::memory_order_release);
        return false;
    }
    return true;
}

double AudioLoader::getDuration() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(total_frames_) / sample_rate_;
}

double AudioLoader::getCurrentTime() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(cursor_.load(std::memory_order_acquire)) / sample_rate_;
}

double AudioLoader::setCurrentTime(double time) {
    if (time < 0.0) time = 0.0;
    double max = getDuration();
    if (time > max) time = max;
    cursor_.store(static_cast<ma_uint64>(time * sample_rate_), std::memory_order_release);
    return time;
}

AudioLoader AudioLoader::clone() const {
    return AudioLoader(filepath_);
}

AudioLoader::PlayStatus AudioLoader::getPlayStatus() const {
    return play_status_.load(std::memory_order_acquire);
}

AudioLoader::LoadStatus AudioLoader::getLoadStatus() const {
    return load_status_;
}

const double AudioLoader::pitch() const { return pitch_; }
double& AudioLoader::pitch() { return pitch_; }
float AudioLoader::volume_linear() const { return volume_.load(std::memory_order_relaxed); }
void AudioLoader::set_volume_linear(float v) { volume_.store(v, std::memory_order_relaxed); }
