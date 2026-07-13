#pragma once
#include "scene/scene.h"
#include "scene/game_scene.h"
#include "scene/menu_scene.h"
#include "game/snake.h"   // for Scene enum values
#include "event/input_event.h"
#include "event/quit_event.h"
#include "raylib.h"

#include <array>
#include <memory>
#include <chrono>
#include <vector>

// -- SceneManager ------------------------------------
// Manages the lifecycle and main loop of all Scenes.
//
// Each frame:
//   ① PollInputEvents() → iterate all raylib input
//   ② Construct InputEvent objects for each pressed key
//   ③ current_scene->handle_event(event)  event dispatch
//   ④ current_scene->update(dt)           logic update
//   ⑤ current_scene->render()             rendering
//
// When Scene::is_finished() returns true,
// automatically switches to the Scene matching get_next_scene_id().
class SceneManager {
public:
    SceneManager();
    ~SceneManager() = default;

    // Run the main loop (blocks until should_quit)
    void run();

private:
    void switch_to(int scene_id);
    void dispatch_input_events();

    std::array<std::unique_ptr<Scene>, 4> scenes_;  // indexed by Scene enum
    int current_scene_ = 0;
    bool should_quit_ = false;

    // timing
    using Clock = std::chrono::steady_clock;
    Clock::time_point last_frame_time_;
};
