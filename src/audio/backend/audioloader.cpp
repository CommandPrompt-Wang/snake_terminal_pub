// 单编译单元：先定义实现宏，再引入单头文件库的实现段
#define DR_MP3_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#include "dr_mp3.h"
#include "miniaudio.h"

#include "audioloader.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

AudioLoader::AudioLoader(std::string filepath)
    : load_status_(LoadStatus::UnknownError)
    , play_status_(PlayStatus::Stopped)
    , pitch_(1.0)
    , volume_(1.0)
{
    // 快速检查文件是否存在（标准 C，跨平台）
    FILE* test = fopen(filepath.c_str(), "rb");
    if (!test) {
        load_status_ = LoadStatus::FileNotFound;
        return;
    }
    fclose(test);

    if (!drmp3_init_file(&audio_data_, filepath.c_str(), nullptr)) {
        load_status_ = LoadStatus::DecoderInitFailed;
        return;
    }

    load_status_ = LoadStatus::Success;
}

AudioLoader::~AudioLoader() {
    stop();
    if (device_initialized_) {
        ma_device_uninit(&device_);
    }
    if (load_status_ == LoadStatus::Success) {
        drmp3_uninit(&audio_data_);
    }
}

AudioLoader::AudioLoader(AudioLoader&& other) noexcept
    : audio_data_(other.audio_data_)
    , device_(other.device_)
    , device_initialized_(other.device_initialized_)
    , load_status_(other.load_status_)
    , play_status_(other.play_status_.load(std::memory_order_acquire))
    , pitch_(other.pitch_)
    , volume_(other.volume_)
{
    std::memset(&other.audio_data_, 0, sizeof(other.audio_data_));
    std::memset(&other.device_, 0, sizeof(other.device_));
    other.device_initialized_ = false;
    other.load_status_ = LoadStatus::UnknownError;
    other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
}

AudioLoader& AudioLoader::operator=(AudioLoader&& other) noexcept {
    if (this != &other) {
        // 先清理自身资源
        if (play_status_.load(std::memory_order_acquire) == PlayStatus::Playing) {
            ma_device_stop(&device_);
        }
        if (device_initialized_) {
            ma_device_uninit(&device_);
        }
        if (load_status_ == LoadStatus::Success) {
            drmp3_uninit(&audio_data_);
        }

        audio_data_ = other.audio_data_;
        device_ = other.device_;
        device_initialized_ = other.device_initialized_;
        load_status_ = other.load_status_;
        play_status_.store(other.play_status_.load(std::memory_order_acquire), std::memory_order_release);
        pitch_ = other.pitch_;
        volume_ = other.volume_;

        std::memset(&other.audio_data_, 0, sizeof(other.audio_data_));
        std::memset(&other.device_, 0, sizeof(other.device_));
        other.device_initialized_ = false;
        other.load_status_ = LoadStatus::UnknownError;
        other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
    }
    return *this;
}

void AudioLoader::play() {
    if (load_status_ != LoadStatus::Success) {
        play_status_.store(PlayStatus::Error, std::memory_order_release);
        return;
    }

    // 首次播放时初始化 miniaudio 设备
    if (!device_initialized_) {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = audio_data_.channels;
        config.sampleRate        = audio_data_.sampleRate;
        config.dataCallback      = ma_data_callback;
        config.pUserData         = this;

        if (ma_device_init(nullptr, &config, &device_) != MA_SUCCESS) {
            load_status_ = LoadStatus::DeviceInitFailed;
            play_status_.store(PlayStatus::Error, std::memory_order_release);
            return;
        }
        device_initialized_ = true;
    }

    if (ma_device_start(&device_) != MA_SUCCESS) {
        load_status_ = LoadStatus::DeviceStartFailed;
        play_status_.store(PlayStatus::Error, std::memory_order_release);
        return;
    }

    play_status_.store(PlayStatus::Playing, std::memory_order_release);
}

void AudioLoader::ma_data_callback(ma_device* pDevice, void* pOutput,
                                    const void* /*pInput*/, ma_uint32 frameCount) {
    auto* self = static_cast<AudioLoader*>(pDevice->pUserData);

    if (self->play_status_.load(std::memory_order_acquire) != PlayStatus::Playing) {
        // 非播放状态：输出静音
        std::memset(pOutput, 0,
                     frameCount * self->audio_data_.channels * sizeof(float));
        return;
    }

    auto* out = static_cast<float*>(pOutput);
    ma_uint64 framesRead = drmp3_read_pcm_frames_f32(
        &self->audio_data_, frameCount, out);

    if (framesRead == 0) {
        // 流结束
        std::memset(pOutput, 0,
                     frameCount * self->audio_data_.channels * sizeof(float));
        self->play_status_.store(PlayStatus::Stopped, std::memory_order_release);
        return;
    }

    // 应用线性音量
    float vol = static_cast<float>(self->volume_);
    if (vol != 1.0f) {
        for (ma_uint64 i = 0; i < framesRead * self->audio_data_.channels; ++i) {
            out[i] *= vol;
        }
    }

    // 不足一帧时尾部补静音
    if (framesRead < frameCount) {
        std::memset(out + framesRead * self->audio_data_.channels, 0,
                     (frameCount - framesRead) * self->audio_data_.channels
                         * sizeof(float));
    }
}

void AudioLoader::pause() {
    auto expected = PlayStatus::Playing;
    if (play_status_.compare_exchange_strong(expected, PlayStatus::Paused,
            std::memory_order_release, std::memory_order_acquire)) {
        ma_device_stop(&device_);
    }
}

void AudioLoader::stop() {
    auto prev = play_status_.exchange(PlayStatus::Stopped, std::memory_order_release);
    if (prev == PlayStatus::Playing || prev == PlayStatus::Paused) {
        ma_device_stop(&device_);
    }
    // 回到开头
    drmp3_seek_to_pcm_frame(&audio_data_, 0);
}

double AudioLoader::getDuration() const {
    if (load_status_ != LoadStatus::Success) return 0.0;
    drmp3_uint64 totalFrames = drmp3_get_pcm_frame_count(const_cast<drmp3*>(&audio_data_));
    if (totalFrames == 0 || audio_data_.sampleRate == 0) return 0.0;
    return static_cast<double>(totalFrames) / audio_data_.sampleRate;
}

double AudioLoader::getCurrentTime() const {
    if (load_status_ != LoadStatus::Success) return 0.0;
    if (audio_data_.sampleRate == 0) return 0.0;
    return static_cast<double>(audio_data_.currentPCMFrame) / audio_data_.sampleRate;
}

double AudioLoader::setCurrentTime(double time) {
    if (load_status_ != LoadStatus::Success) return 0.0;
    if (time < 0.0) time = 0.0;
    drmp3_uint64 targetFrame = static_cast<drmp3_uint64>(time * audio_data_.sampleRate);
    if (!drmp3_seek_to_pcm_frame(&audio_data_, targetFrame)) {
        return getCurrentTime();
    }
    return time;
}

AudioLoader::PlayStatus AudioLoader::getPlayStatus() const {
    return play_status_.load(std::memory_order_acquire);
}

AudioLoader::LoadStatus AudioLoader::getLoadStatus() const {
    return load_status_;
}

const double AudioLoader::pitch() const {
    return pitch_;
}

double& AudioLoader::pitch() {
    return pitch_;
}

const double AudioLoader::volume_linear() const {
    return volume_;
}

double& AudioLoader::volume_linear() {
    return volume_;
}
