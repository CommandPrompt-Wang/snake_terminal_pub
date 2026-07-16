#include "audio/audiomanager.h"

#include <cstring>
#include <utility>

// 主线程安全入口：将命令追加到队列，实际执行延迟到音频回调
void AudioManager::push_command(Command cmd) {
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    cmd_queue_.push_back(std::move(cmd));
}

// 在音频线程执行：swap 出队列后逐条处理，避免持锁混音
void AudioManager::process_commands() {
    std::vector<Command> local;
    {
        std::lock_guard<std::mutex> lock(cmd_mutex_);
        local.swap(cmd_queue_);
    }

    for (auto& cmd : local) {
        switch (cmd.type) {
        case Command::PlaySound: {
            // 互斥：新 BGM 替换旧 BGM
            if (!current_sound_.empty()) {
                auto prev = mp.find(current_sound_);
                if (prev != mp.end()) prev->second.stop();
            }
            auto it = mp.find(cmd.name);
            if (it != mp.end()) {
                it->second.play();
                current_sound_ = cmd.name;
            }
            break;
        }
        case Command::StopSound:
            // 仅停止 loader，ma_device 继续运行
            if (!current_sound_.empty()) {
                auto it = mp.find(current_sound_);
                if (it != mp.end()) it->second.stop();
                current_sound_.clear();
            }
            break;
        case Command::AddSfx:
            if (cmd.sfx && cmd.sfx->isLoadedSuccessfully()) {
                cmd.sfx->play();
                sfx_pool_.push_back(std::move(cmd.sfx));  // 所有权转入 pool，播完由 cleanup_pool 回收
            }
            break;
        case Command::SetVolume:
            // 同时调整模板与正在播放的 SFX 实例
            for (auto& [_, p] : mp) p.set_volume_linear(cmd.volume);
            for (auto& p : sfx_pool_) p->set_volume_linear(cmd.volume);
            if (quit_sfx_) quit_sfx_->set_volume_linear(cmd.volume);
            break;
        case Command::PlayQuitSfx: {
            auto it = mp.find("ui.gameexit");
            if (it == mp.end()) {
                quit_sfx_playing_ = false;
                break;
            }
            quit_sfx_ = std::make_unique<AudioStreamPlayer>(it->second.clone());
            if (!quit_sfx_->isLoadedSuccessfully()) {
                quit_sfx_.reset();
                quit_sfx_playing_ = false;
                break;
            }
            quit_sfx_->play();
            quit_sfx_playing_ = true;
            break;
        }
        }
    }
}

void AudioManager::play_quit_sfx() {
    if (!has_player("ui.gameexit")) {
        quit_sfx_playing_ = false;
        return;
    }
    quit_sfx_playing_ = true;
    Command cmd;
    cmd.type = Command::PlayQuitSfx;
    push_command(std::move(cmd));
}

void AudioManager::cleanup_pool() {
    sfx_pool_.erase(
        std::remove_if(sfx_pool_.begin(), sfx_pool_.end(),
                       [](const auto& p) { return !p->isPlaying(); }),
        sfx_pool_.end());
}

// miniaudio 实时回调：设备常开，无音源时输出静音。
// 禁止在此回调或命令处理中 init/uninit/stop 设备。
void AudioManager::ma_data_callback(ma_device* pDevice, void* pOutput,
                                    const void* /*pInput*/, ma_uint32 frameCount) {
    auto* mgr = static_cast<AudioManager*>(pDevice->pUserData);
    auto* out = static_cast<float*>(pOutput);
    ma_uint32 ch = pDevice->playback.channels;
    ma_uint32 rate = pDevice->sampleRate;

    mgr->process_commands();
    std::memset(out, 0, frameCount * ch * sizeof(float));  // 默认静音，有源再累加

    // 1) 当前 BGM
    if (!mgr->current_sound_.empty()) {
        auto it = mgr->mp.find(mgr->current_sound_);
        if (it != mgr->mp.end()) {
            auto* ld = it->second.get_loader();
            if (ld) ld->mix(out, frameCount, ch, rate);
        }
    }

    // 2) 所有活跃 SFX（累加到同一缓冲区）
    for (auto& p : mgr->sfx_pool_) {
        auto* ld = p->get_loader();
        if (ld) ld->mix(out, frameCount, ch, rate);
    }

    // 3) 退出音效（播完后再允许进程退出）
    if (mgr->quit_sfx_) {
        auto* ld = mgr->quit_sfx_->get_loader();
        if (ld) ld->mix(out, frameCount, ch, rate);
        if (!mgr->quit_sfx_->isPlaying()) {
            mgr->quit_sfx_.reset();
            mgr->quit_sfx_playing_ = false;
        }
    }

    mgr->cleanup_pool();
}
