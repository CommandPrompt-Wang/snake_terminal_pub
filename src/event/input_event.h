#pragma once
#include "event/event.h"

// -- InputEvent -------------------------------------
// Keyboard input event with key code, type (press/release), and repeat flag.
class InputEvent : public Event {
public:
    enum class Type { KeyPress, KeyRelease };

    InputEvent(Type type, int key_code, bool repeat = false)
        : type_(type), key_code_(key_code), repeat_(repeat) {}

    Type get_type() const noexcept { return type_; }
    int get_key_code() const noexcept { return key_code_; }
    bool is_repeat() const noexcept { return repeat_; }

    bool is_key_press() const noexcept { return type_ == Type::KeyPress; }
    bool is_key_release() const noexcept { return type_ == Type::KeyRelease; }

private:
    Type type_;
    int key_code_;
    bool repeat_;
};
