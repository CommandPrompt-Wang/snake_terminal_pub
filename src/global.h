#pragma once

// ── 全局共享状态（跨 Scene 共享，仅保留真正需要跨场景的） ──
namespace Global {
    inline bool should_quit = false;

    inline void request_quit() { should_quit = true; }
    inline bool is_quit_requested() { return should_quit; }

    inline void reset() { should_quit = false; }
} // namespace Global