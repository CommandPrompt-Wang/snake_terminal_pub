#if defined(_WIN32)
extern "C" {
    __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
    __declspec(dllimport) int __stdcall SetConsoleCP(unsigned int);
    __declspec(dllimport) int __stdcall ImmDisableIME(unsigned int);
}
#define CP_UTF8 65001
#endif

#include "global.h"
#include "config/config_loader.h"
#include "scene/scene_manager.h"
#include "audio/audiomanager.h"

#include <algorithm>
#include <clocale>
#include <cstring>
#include <filesystem>
#include <ios>
#include <string>

static std::string tolower_str(const char* s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    // 切到可执行文件所在目录，确保 snake.cfg、resources/ 等相对路径正确
    std::error_code ec;
    std::filesystem::current_path(
        std::filesystem::canonical(argv[0], ec).parent_path(), ec);
    if (ec)
        logw("cannot change to exe directory, cwd unchanged: " + ec.message());

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    ImmDisableIME(-1);
#endif
    std::setlocale(LC_ALL, "C.UTF-8");

    // 解析 --loglevel
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "--loglevel") == 0) {
            auto val = tolower_str(argv[i + 1]);
            if (val == "debug")
                Global::loglevel = LogLevel::Debug;
            else if (val == "info")
                Global::loglevel = LogLevel::Info;
            else if (val == "warning" || val == "warn")
                Global::loglevel = LogLevel::Warning;
            else if (val == "error")
                Global::loglevel = LogLevel::Error;
            // 非法值 → 静默保持 Info
            break;
        }
    }

    log("Welcome to Snake Terminal! Ciallo～(∠・ω< )⌒★");
    log("Loading configuration...");
    // load config – silently falls back to defaults on failure
    load_config("snake.cfg");
    log("Done.");

    log("Initializing audio...");
    AudioStreamPlayer snd_bgm, snd_die, snd_eat, snd_gameover_death, snd_gameover_nondeath;
    AudioStreamPlayer snd_index_switch, snd_toggle, snd_value_assign, snd_enter, snd_back;
    bool load_success = true;
    load_success &= snd_bgm.load("resources/sound/mountain_spider.mp3");
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
        loge("Failed to load one or more audio files. Aborting.");
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
        loge("Failed to initialize audio device.");
        return 1;
    }

    log("Done.");


    // -- 单线程 Scene 主循环 --
    // 所有 raylib 操作（InitWindow、LoadTexture、Draw*）仅在
    // SceneManager::run() 内部的同一个线程上发生，不再有 thread_local 问题。
    SceneManager mgr;
    log("Starting main loop...");
    mgr.run();
    log("Main loop finished.");

    // save config on clean exit
    log("Saving configuration...");
    save_config("snake.cfg");
    log("Done.");

    log("Goodbye!");
    return 0;
}