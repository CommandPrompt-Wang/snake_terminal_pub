#pragma once
#include <atomic>

namespace EventNamespace {
    class EventSrv {
    public:
        enum class Event {
            CONSUMED = -1,

            // 0x move events
            UP = 0,
            DOWN,
            LEFT,
            RIGHT,

            // 1x game events
            APPLE = 10,
            DIE_ON_WALL,
            DIE_ON_SELF,
            DIE_ON_OTHER,

            // 2x game events
            START = 20,
            PAUSE,
            STOP,

            // 2x+ general events
            SCENE_CHANGE = 23,
        };

        enum class EventType {
            MOVE = 0,
            GAME = 1,
            GENERAL = 2,
        };

        Event getEvent(int player, EventType type) {
            switch (type) {
                case EventType::MOVE: {
                    if (player == 1) {
                        return player1Dir.exchange(Event::CONSUMED, std::memory_order_acq_rel);
                    } else {
                        return player2Dir.exchange(Event::CONSUMED, std::memory_order_acq_rel);
                    }
                }
                case EventType::GAME: {
                    if (player == 1) {
                        return player1GameEvent.exchange(Event::CONSUMED, std::memory_order_acq_rel);
                    } else {
                        return player2GameEvent.exchange(Event::CONSUMED, std::memory_order_acq_rel);
                    }
                }
                case EventType::GENERAL: {
                    return generalGameEvent.exchange(Event::CONSUMED, std::memory_order_acq_rel);
                }
                default:
                    return Event::CONSUMED;
            }
        }

        void setDirection(int player, Event dir) {
            if (player == 1) {
                player1Dir.store(dir, std::memory_order_release);
            } else {
                player2Dir.store(dir, std::memory_order_release);
            }
        }

        void setGameEvent(int player, Event event) {
            if (player == 1) {
                player1GameEvent.store(event, std::memory_order_release);
            } else {
                player2GameEvent.store(event, std::memory_order_release);
            }
        }

        void setGeneralEvent(Event event) {
            generalGameEvent.store(event, std::memory_order_release);
        }

        int getSpeedFactor(int player) const {
            return (player == 1) ? player1SpeedFactor.load(std::memory_order_acquire) 
                               : player2SpeedFactor.load(std::memory_order_acquire);
        }

        void setSpeedFactor(int player, int factor) {
            if (player == 1) {
                player1SpeedFactor.store(factor, std::memory_order_release);
            } else {
                player2SpeedFactor.store(factor, std::memory_order_release);
            }
        }

        int getScore(int player) const {
            return (player == 1) ? player1Score.load(std::memory_order_acquire) 
                               : player2Score.load(std::memory_order_acquire);
        }

        void setScore(int player, int score) {
            if (player == 1) {
                player1Score.store(score, std::memory_order_release);
            } else {
                player2Score.store(score, std::memory_order_release);
            }
        }

        int addScore(int player, int points) {
            if (player == 1) {
                return player1Score.fetch_add(points, std::memory_order_acq_rel) + points;
            } else {
                return player2Score.fetch_add(points, std::memory_order_acq_rel) + points;
            }
        }

        int getScene() const {
            return scene.load(std::memory_order_acquire);
        }

        void setScene(int s) {
            scene.store(s, std::memory_order_release);
        }

        void request_quit() {
            should_quit.store(true, std::memory_order_release);
        }

        bool is_quit_requested() const {
            return should_quit.load(std::memory_order_acquire);
        }

        void setApplePosition(int x, int y) {
            appleX.store(x, std::memory_order_release);
            appleY.store(y, std::memory_order_release);
        }

        int getAppleX() const {
            return appleX.load(std::memory_order_acquire);
        }

        int getAppleY() const {
            return appleY.load(std::memory_order_acquire);
        }

        void reset() {
            player1Dir.store(Event::CONSUMED, std::memory_order_release);
            player2Dir.store(Event::CONSUMED, std::memory_order_release);
            
            player1GameEvent.store(Event::CONSUMED, std::memory_order_release);
            player2GameEvent.store(Event::CONSUMED, std::memory_order_release);
            
            generalGameEvent.store(Event::CONSUMED, std::memory_order_release);
            
            player1SpeedFactor.store(1, std::memory_order_release);
            player2SpeedFactor.store(1, std::memory_order_release);
            
            player1Score.store(0, std::memory_order_release);
            player2Score.store(0, std::memory_order_release);

            scene.store(0, std::memory_order_release);  // MENU
        }

    private:
        // direction events: lock-free, only keep the latest
        std::atomic<Event> player1Dir{Event::CONSUMED};
        std::atomic<Event> player2Dir{Event::CONSUMED};

        // game events: lock-free, only keep the latest
        std::atomic<Event> player1GameEvent{Event::CONSUMED};
        std::atomic<Event> player2GameEvent{Event::CONSUMED};

        // general game events: lock-free, only keep the latest
        std::atomic<Event> generalGameEvent{Event::CONSUMED};
        
        // speed factors
        std::atomic<int> player1SpeedFactor{1};
        std::atomic<int> player2SpeedFactor{1};

        // scores
        std::atomic<int> player1Score{0};
        std::atomic<int> player2Score{0};

        // apple position
        std::atomic<int> appleX{-1};
        std::atomic<int> appleY{-1};

        std::atomic<bool> should_quit{false};

        // current scene (Scene enum as int)
        std::atomic<int> scene{0};  // MENU
        
    public:
        static EventSrv& instance() {
            static EventSrv inst;
            return inst;
        }
    };
} // namespace EventNamespace

// easy-access interface for event server
namespace EventServer {
    using Event = EventNamespace::EventSrv::Event;
    using EventType = EventNamespace::EventSrv::EventType;
    
    inline Event getEvent(int player, EventType type) {
        return EventNamespace::EventSrv::instance().getEvent(player, type);
    }
    
    // 1 for normal speed, 2 for double speed, etc.
    inline int getSpeedFactor(int player) {
        return EventNamespace::EventSrv::instance().getSpeedFactor(player);
    }
    
    inline int getScore(int player) {
        return EventNamespace::EventSrv::instance().getScore(player);
    }
    
    inline void setDirection(int player, Event dir) {
        EventNamespace::EventSrv::instance().setDirection(player, dir);
    }
    
    inline void setGameEvent(int player, Event event) {
        EventNamespace::EventSrv::instance().setGameEvent(player, event);
    }
    
    inline void setGeneralEvent(Event event) {
        EventNamespace::EventSrv::instance().setGeneralEvent(event);
    }
    
    // 1 for normal speed, 2 for double speed, etc.
    inline void setSpeedFactor(int player, int factor) {
        EventNamespace::EventSrv::instance().setSpeedFactor(player, factor);
    }
    
    inline void setScore(int player, int score) {
        EventNamespace::EventSrv::instance().setScore(player, score);
    }
    
    inline int addScore(int player, int points) {
        return EventNamespace::EventSrv::instance().addScore(player, points);
    }

    inline void request_quit() {
        EventNamespace::EventSrv::instance().request_quit();
    }

    inline bool is_quit_requested() {
        return EventNamespace::EventSrv::instance().is_quit_requested();
    }

    inline void setApplePosition(int x, int y) {
        EventNamespace::EventSrv::instance().setApplePosition(x, y);
    }

    inline int getAppleX() {
        return EventNamespace::EventSrv::instance().getAppleX();
    }

    inline int getAppleY() {
        return EventNamespace::EventSrv::instance().getAppleY();
    }

    inline int getScene() {
        return EventNamespace::EventSrv::instance().getScene();
    }

    inline void setScene(int s) {
        EventNamespace::EventSrv::instance().setScene(s);
    }
    
    // will consume all events and reset speed factors to 1, scores to 0
    inline void reset() {
        EventNamespace::EventSrv::instance().reset();
    }
}

// export globally
using namespace EventServer;