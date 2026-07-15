#include <iostream>
#include "global.h"
#include "config/config_loader.h"
#include "scene/scene_manager.h"
#include "audio/audiomanager.h"

int main() {
    std::cout << "Welcome to Snake Terminal!" << std::endl;
    std::cout << "Loading configuration..." << std::endl;
    // load config – silently falls back to defaults on failure
    load_config("snake.cfg");
    std::cout << "Done." << std::endl;

    std::cout << "Initializing audio..." << std::endl;
    AudioStreamPlayer snd_bgm, snd_die, snd_eat, snd_gameover_death, snd_gameover_nondeath;
    AudioStreamPlayer snd_index_switch, snd_toggle, snd_value_assign, snd_enter, snd_back;
    bool load_success = true;
    load_success &= snd_bgm.load("resources/sound/Ngau_Hung.mp3");
    load_success &= snd_die.load("resources/sfx/player.die.mp3");
    load_success &= snd_eat.load("resources/sfx/player.eat.mp3");
    load_success &= snd_gameover_death.load("resources/sound/gameover.death.mp3");
    load_success &= snd_gameover_nondeath.load("resources/sound/gameover.nondeath.mp3");
    load_success &= snd_index_switch.load("resources/sfx/ui.index_switch.mp3");
    load_success &= snd_toggle.load("resources/sfx/ui.toggle.mp3");
    load_success &= snd_value_assign.load("resources/sfx/ui.value_assign.mp3");
    load_success &= snd_enter.load("resources/sfx/ui.enter.mp3");
    load_success &= snd_back.load("resources/sfx/ui.back.mp3");

    if (!load_success) {
        std::cerr << "Failed to load one or more audio files. Aborting." << std::endl;
        return 1;
    }

    Global::audio_manager.add_player("bgm", std::move(snd_bgm));
    Global::audio_manager.add_player("die", std::move(snd_die));
    Global::audio_manager.add_player("eat", std::move(snd_eat));
    Global::audio_manager.add_player("gameover.death", std::move(snd_gameover_death));
    Global::audio_manager.add_player("gameover.nondeath", std::move(snd_gameover_nondeath));
    Global::audio_manager.add_player("ui.index_switch", std::move(snd_index_switch));
    Global::audio_manager.add_player("ui.toggle", std::move(snd_toggle));
    Global::audio_manager.add_player("ui.value_assign", std::move(snd_value_assign));
    Global::audio_manager.add_player("ui.enter", std::move(snd_enter));
    Global::audio_manager.add_player("ui.back", std::move(snd_back));

    Global::audio_manager.set_volume_all(game_config().volume);

    if (!Global::audio_manager.init_device()) {
        std::cerr << "Failed to initialize audio device." << std::endl;
        return 1;
    }

    std::cout << "Done." << std::endl;


    // -- 单线程 Scene 主循环 --
    // 所有 raylib 操作（InitWindow、LoadTexture、Draw*）仅在
    // SceneManager::run() 内部的同一个线程上发生，不再有 thread_local 问题。
    SceneManager mgr;
    std::cout << "Starting main loop..." << std::endl;
    mgr.run();
    std::cout << "Main loop finished." << std::endl;

    // save config on clean exit
    std::cout << "Saving configuration..." << std::endl;
    save_config("snake.cfg");
    std::cout << "Done." << std::endl;

    std::cout << "Goodbye!" << std::endl;
    return 0;
}