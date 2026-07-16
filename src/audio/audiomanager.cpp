#include "audio/audiomanager.h"

#include <cstring>
#include <utility>

void AudioManager::push_command(Command cmd) {
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    cmd_queue_.push_back(std::move(cmd));
}

void AudioManager::process_commands() {
    std::vector<Command> local;
    {
        std::lock_guard<std::mutex> lock(cmd_mutex_);
        local.swap(cmd_queue_);
    }

    for (auto& cmd : local) {
        switch (cmd.type) {
        case Command::PlaySound: {
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
            if (!current_sound_.empty()) {
                auto it = mp.find(current_sound_);
                if (it != mp.end()) it->second.stop();
                current_sound_.clear();
            }
            break;
        case Command::AddSfx:
            if (cmd.sfx && cmd.sfx->isLoadedSuccessfully()) {
                cmd.sfx->play();
                sfx_pool_.push_back(std::move(cmd.sfx));
            }
            break;
        case Command::SetVolume:
            for (auto& [_, p] : mp) p.set_volume_linear(cmd.volume);
            for (auto& p : sfx_pool_) p->set_volume_linear(cmd.volume);
            break;
        }
    }
}

void AudioManager::cleanup_pool() {
    sfx_pool_.erase(
        std::remove_if(sfx_pool_.begin(), sfx_pool_.end(),
                       [](const auto& p) { return !p->isPlaying(); }),
        sfx_pool_.end());
}

void AudioManager::ma_data_callback(ma_device* pDevice, void* pOutput,
                                    const void* /*pInput*/, ma_uint32 frameCount) {
    auto* mgr = static_cast<AudioManager*>(pDevice->pUserData);
    auto* out = static_cast<float*>(pOutput);
    ma_uint32 ch = pDevice->playback.channels;
    ma_uint32 rate = pDevice->sampleRate;

    mgr->process_commands();
    std::memset(out, 0, frameCount * ch * sizeof(float));

    if (!mgr->current_sound_.empty()) {
        auto it = mgr->mp.find(mgr->current_sound_);
        if (it != mgr->mp.end()) {
            auto* ld = it->second.get_loader();
            if (ld) ld->mix(out, frameCount, ch, rate);
        }
    }

    for (auto& p : mgr->sfx_pool_) {
        auto* ld = p->get_loader();
        if (ld) ld->mix(out, frameCount, ch, rate);
    }

    mgr->cleanup_pool();
}
