#pragma once

// -- Event base class --------------------------------
// Based on snake_terminal (non-.pub) design.
// All concrete events inherit from this class, controlling the propagation chain via consume().
class Event {
public:
    virtual ~Event() = default;

    void consume() noexcept { consumed_ = true; }
    bool is_consumed() const noexcept { return consumed_; }

private:
    bool consumed_ = false;
};
