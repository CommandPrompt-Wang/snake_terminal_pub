#pragma once

#include <string>

#include "scene/scene.h"
#include "game/snake.h"
#include "raylib.h"

class EndScene : public Scene {
public:
    EndScene() = default;
    ~EndScene() override = default;

    void on_enter() override;
    void on_exit() override;

    void update(float delta_time) override;
    void render() override;

    bool is_finished() const override;
    int get_next_scene_id() const override;
    const char* get_name() const override { return "EndScene"; }

protected:
    void on_inputevent(InputEvent& event) override;

private:
    bool finished_ = false;
    SceneId next_scene_id_ = SceneId::MENU;

    enum class Option {
        RESTART,
        MENU,
        QUIT,
    };
    static constexpr int OPTION_COUNT = 3;
    Option current_option_ = Option::RESTART;

    std::string winner_text_ = "";
    Color winner_color_ = GRAY;

    std::string die_reason_ = "";

    std::string parse_game_over_reason(Global::GameOverReason reason);
};