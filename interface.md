## render_thread 需要实现的

### 1. 初始化 raylib

```cpp
InitWindow(800, 600, "Snake Terminal");
SetTargetFPS(60);
```

### 2. 每帧从 EventServer 读状态

```cpp
// 当前场景
Scene sc = static_cast<Scene>(EventServer::getScene());

// 分数
int score1 = EventServer::getScore(1);
int score2 = EventServer::getScore(2);

// 苹果位置
int ax = EventServer::getAppleX();
int ay = EventServer::getAppleY();

// 方向事件（用于更新自己的 SnakeState）
Event dir1 = EventServer::getEvent(1, EventType::MOVE);
Event dir2 = EventServer::getEvent(2, EventType::MOVE);

// 游戏事件（死亡等）
Event p1ev = EventServer::getEvent(1, EventType::GAME);
Event p2ev = EventServer::getEvent(2, EventType::GAME);

// 通用事件（START / PAUSE / STOP / SCENE_CHANGE）
Event ge = EventServer::getEvent(1, EventType::GENERAL);
```

### 3. 需要自己维护的内容

| 数据 | 说明 |
|---|---|
| `SnakeState p1, p2` | 蛇身 `deque<Position>`、当前方向、分数 |
| 蛇的移动逻辑 | 和 input_thread 一样的 tick 逻辑（读方向推头去尾） |
| 碰撞检测 | 按 `config.toroidalSpace` 决定穿墙还是撞墙 |
| 菜单/配置界面 | 读方向事件导航 |

### 4. 关键常量

```cpp
constexpr int GRID_W = 20;
constexpr int GRID_H = 20;
```

### 5. 配置

```cpp
const auto &cfg = game_config();
cfg.allowAcceleration;       // 是否显示加速效果
cfg.toroidalSpace;           // 环面地图（穿墙绘制）
cfg.allowThroughTeammates;   // 是否允许穿过对方
```
