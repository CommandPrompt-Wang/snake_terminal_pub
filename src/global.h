#pragma once

// ── 全局共享状态（跨 Scene 共享，仅保留真正需要跨场景的） ──
namespace Global {
    inline bool should_quit = false;

    inline int last_score_player1 = 0;
    inline int last_score_player2 = 0;
    enum class GameOverReason {
        NONE = 0,
        PLAYER1_ON_WALL,
        PLAYER2_ON_WALL,
        PLAYER1_ON_SELF,
        PLAYER2_ON_SELF,
        PLAYER1_ON_PLAYER2,
        PLAYER2_ON_PLAYER1,
        PLAYER1_STARVED,
        PLAYER2_STARVED,
        BOTH_STARVED,
    };

    inline GameOverReason last_game_over_reason = GameOverReason::NONE;

    inline void request_quit() { should_quit = true; }
    inline bool is_quit_requested() { return should_quit; }

    inline void reset() { should_quit = false; }
} // namespace Global