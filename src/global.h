#pragma once

#include "audio/audiomanager.h"
#include "utils/log.h"

// -- 全局共享状态（跨 Scene 共享，仅保留真正需要跨场景的） --
namespace Global {
    inline LogLevel loglevel = LogLevel::Info;

    inline bool should_quit = false;
    inline bool quit_pending = false;

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
        DEATH,      // 玩家死亡导致的结束
        TIMEOUT,
        MANUAL,
        FULL_BOARD, // 棋盘被占满，无法生成苹果
    };
    inline GameOverReason end_reason = GameOverReason::NONE;

    inline int end_reason_priority(GameOverReason reason) {
        switch (reason) {
        case GameOverReason::NONE:       return 0;
        case GameOverReason::DEATH:      return 1;
        case GameOverReason::MANUAL:     return 2;
        case GameOverReason::TIMEOUT:    return 3;
        case GameOverReason::FULL_BOARD: return 4;
        }
        return 0;
    }

    inline void set_end_reason(GameOverReason reason) {
        if (end_reason_priority(reason) > end_reason_priority(end_reason))
            end_reason = reason;
    }

    enum class GameMode {
        DEATHMATCH,
        TIMERACE,
    };

    inline GameMode last_game_mode = GameMode::DEATHMATCH;

    inline AudioManager audio_manager;  // 全局单例，场景通过 play_sound / play_sfx 驱动

    inline void request_quit() {
        if (quit_pending || should_quit) return;
        quit_pending = true;
        audio_manager.play_quit_sfx();
    }
    inline bool is_quit_requested() { return should_quit; }
    inline bool is_quit_pending() { return quit_pending; }

    inline void finalize_quit() {
        quit_pending = false;
        should_quit = true;
    }

    inline void reset() {
        should_quit = false;
        quit_pending = false;
    }
} // namespace Global