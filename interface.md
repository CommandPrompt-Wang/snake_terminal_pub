## 架构总览：Scene 回调式单线程

### 目录结构

```
src/
├── app.cpp                  # 入口：加载配置 → SceneManager::run()
├── global.h                 # 全局共享状态（分数、玩家状态、结束原因）
├── utility.h                # 通用类型：Position, Direction, SceneId, GRID_W/H
├── event/                   # Event 类层次
│   ├── event.h              # Event 基类：consume() / is_consumed()
│   ├── input_event.h        # InputEvent(key_code, type, repeat)
│   ├── quit_event.h         # QuitEvent
│   └── scene_switch_event.h # SceneSwitchEvent(next_scene_id)
├── config/                  # 配置系统
│   ├── config.h             # Config 结构体 + game_config()
│   └── config_loader.h      # 配置加载/保存
├── render/                  # 渲染组件
│   ├── draw_list.h          # Draw_List（update/draw 批量管理 + Draw_By_Layer 分层绘制）
│   ├── render.h             # BasicRenderClass 基类
│   └── sprite.h             # Sprite（raylib 纹理精灵，支持 alpha 透明度）
├── scene/                   # 场景系统
│   ├── scene.h              # Scene 基类 + handle_event() 分发
│   ├── scene_manager.h/cpp  # 主循环：输入→事件→更新→渲染
│   ├── menu_scene.h/cpp     # 菜单场景（含 DEATHMATCH / TIMERACE 选择）
│   ├── config_scene.h/cpp   # 配置设置场景（10 项配置）
│   ├── game_scene.h/cpp     # 游戏主场景（蛇移动、碰撞、苹果、复活、虚影打断）
│   └── end_scene.h/cpp      # 结算场景（显示胜者与游戏结束原因）
└── game/                    # 游戏逻辑
    ├── snake.h              # Snake 类（纯游戏逻辑，不感知动画）
    ├── snake.cpp            # Snake 实现
    ├── snake_render.h       # SnakeBlock（蛇身单节精灵） + Vector2 工具函数
    ├── snake_anim.h         # SnakeBody（蛇身渲染体） + 动画状态机
    └── snake_state/
        └── animate_manager.h # AnimateState 基类 + AnimateManager（状态机）
```

### 单线程主循环（SceneManager::run）

```
while (!WindowShouldClose()) {
    PollInputEvents()
    → 遍历按键白名单，构造 InputEvent(key_code, KeyPress/KeyRelease, repeat)
    → current_scene->handle_event(event)
      → on_event(event)             [通用钩子，可 consume 拦截]
      → dynamic_cast → on_inputevent(InputEvent&)
                       on_quitevent(QuitEvent&)
                       on_sceneswitchevent(SceneSwitchEvent&)
    → current_scene->update(dt)
    → if is_finished() → switch_to(next_scene)
    → BeginDrawing()
    → current_scene->render()
    → EndDrawing()
}
```

所有 raylib 调用（InitWindow, LoadTexture, Draw*, BeginDrawing/EndDrawing）
都在同一个线程上，不再有 thread_local 问题。

### Scene 生命周期

| 阶段 | 方法 | 说明 |
|---|---|---|
| 进入 | `on_enter()` | 加载资源、初始化状态 |
| 输入 | `on_inputevent(InputEvent&)` | 由 handle_event 分发 |
| 更新 | `update(dt)` | 游戏逻辑（dt 为秒） |
| 渲染 | `render()` | raylib 绘制（在 BeginDrawing/EndDrawing 内） |
| 退出 | `on_exit()` | 释放资源 |
| 切换 | `is_finished()` / `get_next_scene_id()` | 返回 SceneId |

### 事件处理

```cpp
// Scene 基类
void handle_event(Event& event) {
    if (event.is_consumed()) return;
    on_event(event);           // ① 用户通用钩子
    if (event.is_consumed()) return;
    on_event_internal(event);  // ② dynamic_cast → 具体类型
}

// 子类只需覆盖需要的处理程序
void on_inputevent(InputEvent& event) override {
    if (!event.is_key_press()) return;
    switch (event.get_key_code()) {
        case KEY_W: pending_dir_ = Direction::UP; break;
        case KEY_SPACE: finished_ = true; event.consume(); break;
    }
}
```

### 全局共享状态

```cpp
namespace Global {
    bool should_quit = false;

    int  last_score_player1 = 0;          // 玩家 1 最终得分
    int  last_score_player2 = 0;          // 玩家 2 最终得分

    // 每个玩家的状态（独立记录，不再互相覆盖）
    enum class PlayerStatus {
        ALIVE = -1,  // 存活，不显示
        ON_WALL, ON_SELF, ON_PLAYER, STARVED,
    };
    PlayerStatus player_status1 = PlayerStatus::ALIVE;
    PlayerStatus player_status2 = PlayerStatus::ALIVE;

    // 全局结束原因（非玩家死亡导致的结束）
    enum class GameOverReason {
        DEATH,    // 玩家死亡导致的结束
        TIMEOUT, MANUAL,
    };
    GameOverReason end_reason = GameOverReason::DEATH;

    // 游戏模式
    enum class GameMode { DEATHMATCH, TIMERACE };
    GameMode last_game_mode = GameMode::DEATHMATCH;

    void request_quit();
    bool is_quit_requested();
    void reset();
}
```

结算场景根据 `end_reason` 决定显示：
- `TIMEOUT` → "TIME LIMIT REACHED"
- `MANUAL` → "GAME ENDED"
- `DEATH` → 逐玩家显示 `player_status1`/`player_status2`（跳过 `ALIVE`），至多两行

### 配置

```cpp
const auto &cfg = game_config();
cfg.allowAcceleration;               // 是否加速（Shift 键倍速）
cfg.toroidalSpace;                   // 环面地图（穿墙）
cfg.allowThroughOthers;              // 是否穿过对方
cfg.speed_factor;                    // 整体速度倍率（最小 0.1）
cfg.increasing_difficulty;           // 难度递增系数（0 = 恒定速度）
cfg.time_match_duration;             // TIMERACE 限时秒数（0 = 无穷，UI 显示 inf）
cfg.reborn_costs;                    // TIMERACE 死亡后从尾部切除的节数
cfg.respawnInAdvance;                // 虚影复活模式（死后随机位置生成半透明虚影）
cfg.deathAnimInterruptThreshold;     // 死亡动画打断阈值（身长>此值时允许按键跳过剩余 X 标记）
```

### 蛇-动画分离架构

`Snake` 为纯游戏逻辑类，不包含任何动画状态。动画交由 `SnakeBody` 管理的状态机处理：

```
SnakeBody
  └── AnimateManager（状态机）
        ├── SnakeMove   → 移动 + 加速动画（拥 speedup_offset/frame_process）
        ├── SnakeDie    → 死亡动画（逐节 X 标记，速率同步 tick）
        └── SnakeWaiting → 等待复活（虚影绘制/冻结显示）
```

**状态转换**（由 `game_scene` 驱动）：

| 转换 | 触发 | 说明 |
|---|---|---|
| MOVE → DIE | 蛇死亡 (`apply_death`) | 播放逐节 X 标记 |
| DIE → WAITING | 动画完成 / 按键打断 | 虚影模式部署 ghost |
| WAITING → MOVE | 按键复活 (`handle_respawn`) | 部署真身 |
| MOVE → WAITING | DEATHMATCH 幸存者 | 冻结显示 |

**Ghost 虚影流程**（TIMERACE + `respawnInAdvance`）：
1. 死亡 → 播放死亡动画（原位置 X 标记）
2. 动画完成 → `generate_ghost()`（随机位置，alpha=100 半透明）
3. 玩家看到虚影位置，按键 → `deploy_from_ghost()` → MOVE

**死亡动画打断**（TIMERACE，`body.length > deathAnimInterruptThreshold`）：
- 前 N 节（N=阈值）X 标记正常播放
- 阈值节播完后，按任意移动键跳过剩余 X → 直接 WAITING/虚影

### 游戏模式

| 模式 | 说明 | 死亡处理 |
|---|---|---|
| `DEATHMATCH` | 经典模式，撞墙/撞自己/撞对方即结束 | 死亡方播动画→消失；幸存者冻结→双 WAITING→结算 |
| `TIMERACE` | 计时赛，死亡后复活继续 | 从尾部切除 `reborn_costs` 节；虚影模式生成半透明 ghost；切空则饿死 |

### 控制

| 按键 | 作用 |
|---|---|
| W/A/S/D | 玩家 1 方向 / 死亡动画打断 |
| I/J/K/L | 玩家 2 方向 / 死亡动画打断 |
| L-Shift / R-Shift | 加速（需 `allowAcceleration` 开启） |
| ESC / Backspace | 暂停/继续 |
| T | 暂停时返回菜单 |
| Y | 暂停时手动结束 |
| Shift + A/D | Config 中调整量 ×10 |

### 构建

```bash
./build.sh              # 快速增量构建（不重新配置 CMake/raylib）
./build.sh --build-raylib  # 完整构建（重新 CMake Configure，包含 raylib）
./build.sh --debug         # Debug 模式
./build.sh --debug --build-raylib  # Debug + 完整构建
./build.sh --clean         # 清除 build/ 和 dist/
```
