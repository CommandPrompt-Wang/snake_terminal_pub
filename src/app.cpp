#include <iostream>
#include "global.h"
#include "config/config_loader.h"
#include "scene/scene_manager.h"

int main() {
    // load config – silently falls back to defaults on failure
    load_config("snake.cfg");

    // -- 单线程 Scene 主循环 --
    // 所有 raylib 操作（InitWindow、LoadTexture、Draw*）仅在
    // SceneManager::run() 内部的同一个线程上发生，不再有 thread_local 问题。
    SceneManager mgr;
    mgr.run();

    // save config on clean exit
    // render_thread();
    save_config("snake.cfg");

    std::cout << "Goodbye!" << std::endl;
    return 0;
}