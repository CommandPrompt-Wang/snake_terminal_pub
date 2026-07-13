#include "scene/scene_manager.h"
#include "scene/end_scene.h"
#include "scene/config_scene.h"
#include "global.h"
#include "event/input_event.h"
#include "event/quit_event.h"
#include "event/scene_switch_event.h"
#include "config/config.h"
#include "raylib.h"

#include <iostream>

SceneManager::SceneManager() {
    // -- Register all Scenes --
    // Indices correspond to Scene enum: 0=MENU, 1=CONFIG, 2=GAME, 3=DIE
    scenes_[static_cast<int>(SceneId::MENU)] = std::make_unique<MenuScene>();
    scenes_[static_cast<int>(SceneId::CONFIG)] = std::make_unique<ConfigScene>();
    scenes_[static_cast<int>(SceneId::GAME)]  = std::make_unique<GameScene>();
    scenes_[static_cast<int>(SceneId::DIE)]   = std::make_unique<EndScene>();

    current_scene_ = static_cast<int>(SceneId::MENU);
    last_frame_time_ = Clock::now();
}

void SceneManager::switch_to(int scene_id) {
    if (scene_id < 0 || scene_id >= static_cast<int>(scenes_.size())) return;
    if (!scenes_[scene_id]) return;

    // exit current scene
    if (scenes_[current_scene_]) {
        scenes_[current_scene_]->on_exit();
    }

    current_scene_ = scene_id;

    // enter new scene
    scenes_[current_scene_]->on_enter();
}

// -- Convert raylib input to InputEvent objects and dispatch to current Scene --
void SceneManager::dispatch_input_events() {
    Scene* current = scenes_[current_scene_].get();
    if (!current) return;

    // Define the list of keys to monitor
    static const int keys_to_check[] = {
        KEY_W, KEY_S, KEY_A, KEY_D,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
        KEY_SPACE, KEY_ESCAPE, KEY_SLASH, KEY_ENTER, KEY_P, KEY_C,
    };

    for (int key : keys_to_check) {
        if (IsKeyPressed(key)) {
            InputEvent ev(InputEvent::Type::KeyPress, key, false);
            current->handle_event(ev);
            // If the scene consumed the event, continue to next
        }
        if (IsKeyReleased(key)) {
            InputEvent ev(InputEvent::Type::KeyRelease, key, false);
            current->handle_event(ev);
        }
    }
}

void SceneManager::run() {
    // -- Initialize raylib window --
    // This is the only place in the entire program where a window is created
    InitWindow(640, 840, "Snake Terminal");
    SetExitKey(0);  // disable raylib's default ESC→close, handled per-scene
    SetTargetFPS(60);

    // Start the initial scene
    if (scenes_[current_scene_]) {
        scenes_[current_scene_]->on_enter();
    }

    last_frame_time_ = Clock::now();

    // -- Main loop --
    while (!WindowShouldClose() && !should_quit_ && !Global::is_quit_requested()) {
        // Calculate delta time
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - last_frame_time_).count();
        last_frame_time_ = now;

        // ① Construct event objects and dispatch to current Scene
        // NOTE: PollInputEvents() is already called inside EndDrawing() at end of previous frame.
        //       Do NOT call it here again — it would reset key edge-detection and drop inputs.
        dispatch_input_events();

        // ③ Check if QuitEvent was triggered
        if (Global::is_quit_requested()) break;

        Scene* current = scenes_[current_scene_].get();
        if (!current) break;

        // ④ Logic update
        current->update(dt);

        // ⑤ Check scene switch
        if (current->is_finished()) {
            int next = current->get_next_scene_id();

            // DIE → MENU: if DIE scene is not registered, fall back to MENU
            if (next == static_cast<int>(SceneId::DIE) && !scenes_[static_cast<int>(SceneId::DIE)]) {
                next = static_cast<int>(SceneId::MENU);
            }

            switch_to(next);
            current = scenes_[current_scene_].get();
        }

        // ⑥ Render (all raylib calls happen here)
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (current) {
            current->render();
        }

        EndDrawing();
    }

    // -- Cleanup --
    for (auto &s : scenes_) {
        if (s) s->on_exit();
    }

    CloseWindow();
}
