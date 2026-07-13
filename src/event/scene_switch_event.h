#pragma once
#include "event/event.h"

// -- SceneSwitchEvent -------------------------------
// Carries the target scene ID (matching the Scene enum value).
class SceneSwitchEvent : public Event {
public:
    explicit SceneSwitchEvent(int next_scene_id)
        : next_scene_id_(next_scene_id) {}

    int get_next_scene_id() const { return next_scene_id_; }

private:
    int next_scene_id_;
};
