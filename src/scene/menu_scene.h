#pragma once
#include "scene/scene.h"
#include "utility.h"

// -- MenuScene --------------------------------------
// Menu scene: displays title, option list with up/down navigation.
class MenuScene : public Scene {
public:
    MenuScene() = default;
    ~MenuScene() override = default;

    void on_enter() override;
    void on_exit() override;
    void update(float dt) override;
    void render() override;

    bool is_finished() const override { return finished_; }
    int get_next_scene_id() const override { return next_scene_id_; }
    const char* get_name() const override { return "MenuScene"; }

protected:
    void on_inputevent(InputEvent& event) override;
    void on_quitevent(QuitEvent& event) override;

private:
    enum class Option {
        START_DEATHMATCH,
        START_TIMERACE,
        CONFIG,
        QUIT,
    };
    static constexpr int OPTION_COUNT = 4;
    Option current_option_ = Option::START_DEATHMATCH;
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::GAME);
};
