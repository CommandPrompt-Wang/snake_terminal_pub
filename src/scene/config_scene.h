#pragma once

#include "scene/scene.h"
#include "utility.h"
#include "raylib.h"

class ConfigScene : public Scene {
public:
    ConfigScene() = default;
    ~ConfigScene() override = default;

    void on_enter() override;
    void on_exit() override;
    void update(float delta_time) override;
    void render() override;

    bool is_finished() const override;
    int get_next_scene_id() const override;
    const char* get_name() const override { return "ConfigScene"; }

protected:
    void on_inputevent(InputEvent& event) override;

private:
    enum class Option {
        ALLOW_ACCELERATION,
        TOROIDAL_SPACE,
        ALLOW_THROUGH_OTHERS,
        SPEED_FACTOR,
        INCREASING_DIFFICULTY,
        TIME_MATCH_DURATION,
        REBORN_COSTS,
        RESPAWN_IN_ADVANCE,
        BACK,
    };
    static constexpr int OPTION_COUNT = 9;
    Option current_option_ = Option::ALLOW_ACCELERATION;
    bool finished_ = false;
    int next_scene_id_ = static_cast<int>(SceneId::MENU);
};