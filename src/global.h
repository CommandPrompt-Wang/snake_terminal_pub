#pragma once

// ── 全局共享状态（跨 Scene 共享，仅保留真正需要跨场景的） ──
namespace Global {
    inline bool should_quit = false;

    inline int last_score_player1 = 0;
    inline int last_score_player2 = 0;

    // 每个玩家的状态
    enum class PlayerStatus {
        ALIVE = -1,  // 存活，不显示
        NONE = 0,
        ON_WALL,
        ON_SELF,
        ON_PLAYER,
        STARVED,
    };
    inline PlayerStatus player_status1 = PlayerStatus::ALIVE;
    inline PlayerStatus player_status2 = PlayerStatus::ALIVE;

    // 全局结束原因（非玩家死亡导致的结束）
    enum class GameOverReason {
        NONE = 0,
        DEATH,    // 玩家死亡导致的结束
        TIMEOUT,
        MANUAL,
    };
    inline GameOverReason end_reason = GameOverReason::NONE;

    enum class GameMode {
        DEATHMATCH,
        TIMERACE,
    };

    inline GameMode last_game_mode = GameMode::DEATHMATCH;  

    inline void request_quit() { should_quit = true; }
    inline bool is_quit_requested() { return should_quit; }

    inline void reset() { should_quit = false; }
} // namespace Global