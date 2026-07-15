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
{
    FILE* test = fopen(filepath.c_str(), "rb");
    if (!test) { load_status_ = LoadStatus::FileNotFound; return; }
    fclose(test);

    drmp3 mp3;
    if (!drmp3_init_file(&mp3, filepath.c_str(), nullptr)) {
        load_status_ = LoadStatus::DecoderInitFailed; return;
    }

    channels_ = mp3.channels;
    sample_rate_ = mp3.sampleRate;
    total_frames_ = drmp3_get_pcm_frame_count(&mp3);

    pcm_buffer_.resize(total_frames_ * channels_);
    drmp3_read_pcm_frames_f32(&mp3, total_frames_, pcm_buffer_.data());
    drmp3_uninit(&mp3);

    load_status_ = LoadStatus::Success;
}

AudioLoader::~AudioLoader() {
    stop();
    if (device_initialized_) ma_device_uninit(&device_);
}

AudioLoader::AudioLoader(AudioLoader&& other) noexcept
    : device_(other.device_)
    , device_initialized_(other.device_initialized_)
    , load_status_(other.load_status_)
    , play_status_(other.play_status_.load(std::memory_order_acquire))
    , pcm_buffer_(std::move(other.pcm_buffer_))
    , total_frames_(other.total_frames_)
    , cursor_(other.cursor_)
    , channels_(other.channels_)
    , sample_rate_(other.sample_rate_)
{
    std::memset(&other.device_, 0, sizeof(other.device_));
    other.device_initialized_ = false;
    other.load_status_ = LoadStatus::UnknownError;
    other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
    other.total_frames_ = 0;
    other.cursor_ = 0;
}

AudioLoader& AudioLoader::operator=(AudioLoader&& other) noexcept {
    if (this != &other) {
        if (play_status_.load(std::memory_order_acquire) == PlayStatus::Playing)
            ma_device_stop(&device_);
        if (device_initialized_) ma_device_uninit(&device_);

        device_ = other.device_;
        device_initialized_ = other.device_initialized_;
        load_status_ = other.load_status_;
        play_status_.store(other.play_status_.load(std::memory_order_acquire), std::memory_order_release);
        pcm_buffer_ = std::move(other.pcm_buffer_);
        total_frames_ = other.total_frames_;
        cursor_ = other.cursor_;
        channels_ = other.channels_;
        sample_rate_ = other.sample_rate_;

        std::memset(&other.device_, 0, sizeof(other.device_));
        other.device_initialized_ = false;
        other.load_status_ = LoadStatus::UnknownError;
        other.play_status_.store(PlayStatus::Stopped, std::memory_order_release);
        other.total_frames_ = 0;
        other.cursor_ = 0;
    }
    return *this;
}

void AudioLoader::play() {
    if (load_status_ != LoadStatus::Success) return;
    cursor_ = 0;

    if (!device_initialized_) {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = channels_;
        config.sampleRate        = sample_rate_;
        config.dataCallback      = ma_data_callback;
        config.pUserData         = this;
        if (ma_device_init(nullptr, &config, &device_) != MA_SUCCESS) return;
        device_initialized_ = true;
    }

    ma_device_start(&device_);
    play_status_.store(PlayStatus::Playing, std::memory_order_release);
}

void AudioLoader::ma_data_callback(ma_device* pDevice, void* pOutput,
                                    const void* /*pInput*/, ma_uint32 frameCount) {
    auto* self = static_cast<AudioLoader*>(pDevice->pUserData);

    if (self->play_status_.load(std::memory_order_acquire) != PlayStatus::Playing) {
        std::memset(pOutput, 0, frameCount * self->channels_ * sizeof(float));
        return;
    }

    auto* out = static_cast<float*>(pOutput);
    ma_uint64 remain = self->total_frames_ - self->cursor_;
    ma_uint64 n = frameCount;
    if (n > remain) n = remain;

    std::memcpy(out, self->pcm_buffer_.data() + self->cursor_ * self->channels_,
                n * self->channels_ * sizeof(float));
    self->cursor_ += n;

    if (n < frameCount)
        std::memset(out + n * self->channels_, 0,
                    (frameCount - n) * self->channels_ * sizeof(float));
}

void AudioLoader::resume() { play(); }

void AudioLoader::pause() { stop(); }

void AudioLoader::stop() {
    auto prev = play_status_.exchange(PlayStatus::Stopped, std::memory_order_release);
    if (prev == PlayStatus::Playing || prev == PlayStatus::Paused)
        ma_device_stop(&device_);
    cursor_ = 0;
}

double AudioLoader::getDuration() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(total_frames_) / sample_rate_;
}

double AudioLoader::getCurrentTime() const {
    if (sample_rate_ == 0) return 0.0;
    return static_cast<double>(cursor_) / sample_rate_;
}

double AudioLoader::setCurrentTime(double time) {
    if (time < 0.0) time = 0.0;
    double max = getDuration();
    if (time > max) time = max;
    cursor_ = static_cast<ma_uint64>(time * sample_rate_);
    return time;
}

AudioLoader::PlayStatus AudioLoader::getPlayStatus() const {
    return play_status_.load(std::memory_order_acquire);
}

AudioLoader::LoadStatus AudioLoader::getLoadStatus() const {
    return load_status_;
}

const double AudioLoader::pitch() const { return pitch_; }
double& AudioLoader::pitch() { return pitch_; }
const double AudioLoader::volume_linear() const { return volume_; }
double& AudioLoader::volume_linear() { return volume_; }
