#include <iostream>
#include "global.h"
#include "config/config_loader.h"
#include "scene/scene_manager.h"

int main() {
    std::cout << "Welcome to Snake Terminal!" << std::endl;
    std::cout << "Loading configuration..." << std::endl;
    // load config – silently falls back to defaults on failure
    load_config("snake.cfg");
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