#pragma once
#include "event/event.h"
#include "event/input_event.h"
#include "event/quit_event.h"
#include "event/scene_switch_event.h"

// -- Scene abstract base class ----------------------
// Based on snake_terminal (non-.pub) Scene + Event design.
//
// Key change: instead of polling state via a global EventServer,
// SceneManager creates concrete Event objects
// and dispatches them via handle_event() → dynamic_cast.
//
// Main loop each frame:
//   create events → scene.handle_event(event)
//                → scene.update(dt)
//                → scene.render()
class Scene {
public:
    virtual ~Scene() = default;

    // ========== Lifecycle ==========
    virtual void on_enter() {}   // Called when entering the scene (load resources, etc.)
    virtual void on_exit() {}    // Called when leaving the scene (release resources, etc.)

    // ========== Main Loop ==========
    virtual void update(float delta_time) = 0;  // Game logic
    virtual void render() = 0;                  // Render frame

    // ========== Event Handling ==========
    // Two-phase event dispatch:
    //   ① on_event(event) — generic hook, can consume to intercept here
    //   ② dynamic_cast → specific type handler
    void handle_event(Event& event) {
        if (event.is_consumed()) return;
        on_event(event);
        if (event.is_consumed()) return;
        on_event_internal(event);
    }

    // ========== State Queries ==========
    virtual bool is_finished() const { return false; }
    virtual int get_next_scene_id() const { return 0; }  // 0 = MENU
    virtual const char* get_name() const = 0;

    // ========== Resource Management ==========
    virtual void load_resources() {}
    virtual void unload_resources() {}

protected:
    // Generic event hook (can be overridden in subclasses, takes priority over type dispatch)
    virtual void on_event(Event& event) { (void)event; }

    // Type-safe event handlers
    virtual void on_inputevent(InputEvent& event) { (void)event; }
    virtual void on_quitevent(QuitEvent& event) { (void)event; }
    virtual void on_sceneswitchevent(SceneSwitchEvent& event) { (void)event; }

private:
    void on_event_internal(Event& event) {
        if (auto* e = dynamic_cast<InputEvent*>(&event)) {
            on_inputevent(*e);
            return;
        }
        if (auto* e = dynamic_cast<QuitEvent*>(&event)) {
            on_quitevent(*e);
            return;
        }
        if (auto* e = dynamic_cast<SceneSwitchEvent*>(&event)) {
            on_sceneswitchevent(*e);
            return;
        }
    }
};
