## 架构总览：Scene 回调式单线程

### 目录结构

```
src/
├── app.cpp                  # 入口：加载配置 → SceneManager::run()
├── global.h                 # 全局共享状态（分数、游戏结束原因）
├── utility.h                # 通用类型：Position, Direction, SceneId, GRID_W/H
├── event/                   # Event 类层次（参考 snake_terminal）
│   ├── event.h              # Event 基类：consume() / is_consumed()
│   ├── input_event.h        # InputEvent(key_code, type, repeat)
│   ├── quit_event.h         # QuitEvent
│   └── scene_switch_event.h # SceneSwitchEvent(next_scene_id)
├── config/                  # 配置系统
│   ├── config.h             # Config 结构体 + game_config()
│   └── config_loader.h      # 配置加载/保存
├── render/                  # 渲染组件
│   ├── draw_list.h          # Draw_List（update/draw 批量管理）
│   ├── render.h             # Basic_Render_Class 基类
│   └── sprite.h             # Sprite（raylib 纹理精灵）
├── scene/                   # 场景系统
│   ├── scene.h              # Scene 基类 + handle_event() 分发
│   ├── scene_manager.h/cpp  # 主循环：输入→事件→更新→渲染
│   ├── menu_scene.h/cpp     # 菜单场景
│   ├── config_scene.h/cpp   # 配置设置场景
│   ├── game_scene.h/cpp     # 游戏主场景（蛇移动、碰撞、苹果、穿墙）
│   └── end_scene.h/cpp      # 结算场景（显示胜者与游戏结束原因）
└── game/                    # 游戏逻辑
    ├── snake.h              # SnakeState, random_apple_pos
    └── snake_render.h       # Snake_Block / Snake_Body（蛇身精灵渲染）
```

### 单线程主循环（SceneManager::run）

```
while (!WindowShouldClose()) {
    PollInputEvents()
    → 遍历按键，构造 InputEvent(key_code, KeyPress/KeyRelease, repeat)
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

### 事件处理（参考 snake_terminal）

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

    enum class GameOverReason {
        NONE,
        PLAYER1_ON_WALL,   PLAYER2_ON_WALL,
        PLAYER1_ON_SELF,   PLAYER2_ON_SELF,
        PLAYER1_ON_PLAYER2, PLAYER2_ON_PLAYER1,
        PLAYER1_STARVED,   PLAYER2_STARVED,
        BOTH_STARVED,
    };
    GameOverReason last_game_over_reason = GameOverReason::NONE;

    void request_quit();
    bool is_quit_requested();
    void reset();
}
```

### 配置

```cpp
const auto &cfg = game_config();
cfg.allowAcceleration;        // 是否加速
cfg.toroidalSpace;            // 环面地图（穿墙）
cfg.allowThroughTeammates;    // 是否穿过对方
cfg.speed_factor;             // 整体速度倍率（最小 0.1）
cfg.increasing_difficulty;    // 难度递增系数（0 = 恒定速度）
```
