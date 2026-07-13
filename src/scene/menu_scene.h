#pragma once
#include "scene/scene.h"
#include "game/snake.h"

// -- MenuScene --------------------------------------
// Menu scene: displays title, waits for SPACE to start the game
// Input is dispatched via handle_event() → on_inputevent()
class MenuScene : public Scene {
public:
    MenuScene() = default;
    ~MenuScene() override = default;

    void on_enter() override;
    void on_exit() override;
    void update(float dt) override;
    void render() override;

    bool is_finished() const override { return finished_; }
    int get_next_scene_id() const override { return static_cast<int>(SceneId::GAME); }
    const char* get_name() const override { return "MenuScene"; }

protected:
    void on_inputevent(InputEvent& event) override;
    void on_quitevent(QuitEvent& event) override;

private:
    bool finished_ = false;
};
