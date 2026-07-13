#pragma once

#include "scene/scene.h"
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

}