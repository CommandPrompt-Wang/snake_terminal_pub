# Snake Terminal

双人本地对战贪吃蛇，基于 [raylib](https://www.raylib.com/) 与 C++17 实现。支持经典淘汰模式与限时竞速模式，可自定义地图规则、复活机制与难度曲线。

## 特性

- **双人对战**：两名玩家同屏竞技，各自独立 tick 计时
- **两种游戏模式**
  - **DEATHMATCH**：撞墙 / 撞自己 / 撞对方即出局，幸存者获胜
  - **TIMERACE**：限时计分，死亡后扣除尾部节数并复活继续
- **丰富配置**：环面地图、穿身、加速、虚影复活、难度递增等 9 项可调参数
- **场景驱动架构**：菜单 → 配置 → 游戏 → 结算，事件回调式单线程主循环
- **自研音频系统**：miniaudio 预解码 PCM + 回调混音，独立于主线程，不受帧率波动影响
- **跨平台构建**：Linux（GCC/Clang）与 Windows（MinGW）一键脚本
- **一键打包**：Linux AppImage / Windows 单文件 exe，便于分发

## 快速开始

### 依赖

| 平台 | 要求 |
|------|------|
| Linux | CMake ≥ 3.16、C++17 编译器、Git |
| Windows | CMake ≥ 3.16、MinGW-w64、Git |
| Linux（打包） | [AppImageTool](https://github.com/AppImage/AppImageKit/releases) |
| Windows（打包） | [Enigma Virtual Box](https://enigmaprotector.com/en/aboutvb.html) |

首次克隆需拉取子模块（raylib 等）：

```bash
git clone --recurse-submodules <repo-url>
cd snake_terminal.pub
```

若已克隆但未初始化子模块：

```bash
git submodule update --init --recursive
```

### 构建与运行

**Linux**

`build.bat` 与 `pack.bat` 是 **polyglot 黑魔法**——同一个文件在 Windows 下当批处理、在 Linux/macOS 下当 shell 脚本，自动路由到对应平台的实现。Linux 下用法：

```bash
bash ./build.bat              # 或 ./build.bat
bash ./pack.bat
```

参数会透传（如 `--debug`、`--clean`、`--build-raylib`）。

> **出问题时用分离脚本。** 黑魔法依赖混用 CRLF/LF 换行：batch 段（至 `END_WIN` 含）须 **CRLF**，shell 段须 **LF**。若 shell 段被编辑器保存成 CRLF，WSL 下路径会带 `\r` 导致 `build-linux.sh: 没有那个文件或目录`。`.gitattributes` 中已对 `build.bat` / `pack.bat` 设 `-text` 防止 Git 自动转换。
>
> | 用途 | Linux | Windows |
> |------|-------|---------|
> | 构建 | `build-linux.sh` | `build-windows.bat` |
> | 打包 | `pack-linux.sh` | `pack-windows.bat` |

```bash
./build-linux.sh              # 等价于 bash ./build.bat
./build-linux.sh --debug      # Debug 模式
./build-linux.sh --build-raylib  # 完整构建（含 raylib 重编译）
./build-linux.sh --clean      # 清除 build-linux/ 和 dist-linux/

./dist-linux/snake            # 运行
```

**Windows（MinGW）**

`build.bat` / `pack.bat` 在 Windows 下同样可用；若异常，改用 `build-windows.bat` / `pack-windows.bat`。

```bat
build.bat                     :: Release 构建
build.bat --debug             :: Debug 模式

dist-windows\snake.exe        :: 运行
```

构建产物会自动复制 `resources/` 到输出目录。程序启动时会将工作目录切换到可执行文件（或 AppImage 的 `APPDIR` 挂载点），确保 `resources/` 路径正确。配置文件 `snake.cfg` 的读写位置见下方「配置」一节。

### 打包分发

先完成构建，再运行对应平台的打包脚本，将可执行文件与 `resources/` 打成单个分发包。

**Linux — AppImage**

依赖 [AppImageTool](https://github.com/AppImage/AppImageKit/releases)（默认路径 `~/appimagetool-x86_64.AppImage`，可通过环境变量 `APPIMAGETOOL_PATH` 覆盖）：

```bash
./build-linux.sh              # 或 bash ./build.bat
./pack-linux.sh               # 或 bash ./pack.bat

./pack-linux/snake-x86_64.AppImage    # 运行
```

脚本会自动收集运行时 `.so` 依赖、生成 `.desktop` 与 `AppRun`，输出至 `pack-linux/snake-x86_64.AppImage`。

**Windows — 单文件 exe**

依赖 [Enigma Virtual Box](https://enigmaprotector.com/en/aboutvb.html)（默认安装路径 `C:\Program Files (x86)\Enigma Virtual Box\`）。

**首次打包前**须用 Enigma VB GUI 打开 `pack-windows.evb`，将 `InputFile`、`OutputFile` 及所有资源文件的 `File` 路径改为你本机仓库的**绝对路径**。已尝试过相对路径，Enigma VB 不生效，因此只能写绝对路径；仓库里的 `.evb` 仅作模板，路径仍指向作者机器。

```bat
build-windows.bat
pack-windows.bat              :: 或 build.bat / pack.bat

pack-windows\snake-packed.exe   :: 运行
```

Enigma VB 将 `dist-windows\snake.exe` 与 `resources/` 虚拟化打包为单个 exe，运行时解压到临时目录，无需额外携带资源文件夹。

### 命令行

```bash
./dist-linux/snake --loglevel debug    # 日志级别：debug / info / warning / error（默认 info）
```

Windows 下额外设置控制台 UTF-8 编码并禁用 IME 输入法。

## 操作说明

| 按键 | 作用 |
|------|------|
| W / A / S / D | 玩家 1 方向 / 死亡动画打断 |
| I / J / K / L | 玩家 2 方向 / 死亡动画打断 |
| L-Shift / R-Shift | 加速（tick 间隔减半，需开启 `allowAcceleration`） |
| ESC / Backspace | 暂停 / 继续 |
| T | 暂停时返回菜单 |
| Y | 暂停时手动结束 |
| Shift + A/D 或 J/L | 配置界面中调整量 ×10 |

## 游戏模式

| 模式 | 说明 | 死亡处理 |
|------|------|----------|
| `DEATHMATCH` | 经典模式，撞墙 / 撞自己 / 撞对方即结束 | 死亡方播动画后消失；幸存者冻结 → 结算 |
| `TIMERACE` | 计时赛，死亡后复活继续 | 从尾部切除 `reborn_costs` 节（见下节）；可开启复活前预先显示透明身体；切空则饿死 |

**Tick 计时**：两玩家各有独立 tick 累积器，不再共享时钟。Shift 加速按下时 tick 间隔减半。碰撞检测按 tick 标志分别判定，同时死亡时双方均触发死亡处理。

## 配置

游戏设置保存在 `snake.cfg`（纯文本）。若文件不存在则自动创建默认配置；缺少字段时用默认值补全，正常退出时写回完整文件。

**配置文件位置**（按优先级）：

| 场景 | 路径 |
|------|------|
| 普通可执行文件 | 与 `snake` / `snake.exe` 同目录 |
| AppImage | 与 `.AppImage` 文件同目录（通过 `APPIMAGE` / `ARGV0` 解析） |
| AppImage 目录不可写 | `~/.config/snake_terminal/snake.cfg`（XDG） |

启动时会日志输出 `config file: …` 显示实际使用的路径。

| 字段 | 说明 |
|------|------|
| `volume` | 音量 0–100 |
| `allowAcceleration` | 是否允许 Shift 加速 |
| `toroidalSpace` | 环面地图（穿墙） |
| `allowThroughOthers` | 是否可穿过对方 |
| `speed_factor` | 整体速度倍率（最小 0.1） |
| `increasing_difficulty` | 难度递增系数（0 = 恒定速度） |
| `time_match_duration` | TIMERACE 限时秒数（0 = 无穷） |
| `reborn_costs` | TIMERACE 死亡后从尾部切除的节数 |
| `respawnInAdvance` | 复活前预先显示透明身体，按移动键立即部署 |
| `deathAnimInterruptThreshold` | 当蛇身体超过该值时，可以跳过超出部分的死亡动画（-1 = 关闭） |

也可在游戏内通过 **Config** 场景可视化调整。

## 项目结构

```
snake_terminal.pub/
├── src/                      # 源代码
├── resources/                # 纹理、音效等资源
├── third_party/              # raylib、miniaudio、dr_libs 子模块
├── build.bat                 # 跨平台 polyglot 入口（黑魔法，见上文）
├── build-linux.sh            # Linux 构建（推荐）
├── build-windows.bat         # Windows 构建（推荐）
├── pack.bat                  # 跨平台 polyglot 入口（黑魔法，见上文）
├── pack-linux.sh             # Linux AppImage 打包（推荐）
├── pack-windows.bat          # Windows Enigma VB 打包（推荐）
├── pack-windows.evb          # Windows Enigma VB 打包配置
├── CMakeLists.txt
├── dist-linux/               # Linux 构建输出
├── dist-windows/             # Windows 构建输出
├── pack-linux/               # Linux AppImage 输出
└── pack-windows/             # Windows 单文件 exe 输出
```

## 技术栈

- **渲染**：[raylib](https://github.com/raysan5/raylib) 5.x
- **音频**：[miniaudio](https://github.com/mackron/miniaudio) + [dr_mp3](https://github.com/mackron/dr_libs)
- **语言 / 标准**：C++17
- **构建**：CMake 3.16+

---

## 架构文档

以下为项目内部设计与实现细节，供开发者参考。

### 目录结构

```
src/
├── app.cpp                  # 入口：切工作目录→解析 CLI→加载配置→音频初始化→SceneManager::run()
├── global.h                 # 全局共享状态（分数、玩家状态、结束原因、AudioManager）
├── utility.h                # 通用类型：Position, Direction, SceneId, GRID_W/H
├── event/                   # Event 类层次
│   ├── event.h              # Event 基类：consume() / is_consumed()
│   ├── input_event.h        # InputEvent(key_code, type, repeat)
│   ├── quit_event.h         # QuitEvent
│   └── scene_switch_event.h # SceneSwitchEvent(next_scene_id)
├── config/                  # 配置系统
│   ├── config.h             # Config 结构体（含 volume 0-100）+ game_config()
│   ├── config_loader.h      # 配置加载/保存（声明）
│   └── config_loader.cpp    # 配置加载/保存（实现）
├── audio/                   # 音频系统
│   ├── audiomanager.h       # AudioManager：共享 ma_device + play_sound/play_sfx/stop_sound
│   ├── audiostreamplayer.h  # AudioStreamPlayer：包装 AudioLoader，load/play/stop/clone
│   └── backend/
│       ├── audioloader.h    # AudioLoader：预解码 PCM + mix() 混音（含线性插值重采样）
│       └── audioloader.cpp  # 实现（单编译单元含 dr_mp3 + miniaudio IMPL）
├── render/                  # 渲染组件
│   ├── drawlist.h          # DrawList（update/draw 批量管理 + DrawLayer 分层绘制）
│   ├── render.h             # BasicRenderClass 基类
│   └── sprite.h             # Sprite（raylib 纹理精灵，支持 alpha 透明度）
├── utils/                   # 工具
│   ├── log.h                # 日志系统（线程安全、彩色输出、级别过滤、raylib 接管）
│   └── log.cpp              # 日志实现
├── scene/                   # 场景系统
│   ├── scene.h              # Scene 基类 + handle_event() 分发
│   ├── scene_manager.h/cpp  # 主循环：输入→事件→更新→渲染
│   ├── menu_scene.h/cpp     # 菜单场景（含 DEATHMATCH / TIMERACE 选择）
│   ├── config_scene.h/cpp   # 配置设置场景（音量 + 9 项配置）
│   ├── game_scene.h/cpp     # 游戏主场景（独立 tick 计时、碰撞、苹果、复活、虚影打断）
│   └── end_scene.h/cpp      # 结算场景（显示胜者与游戏结束原因）
└── game/                    # 游戏逻辑
    ├── snake.h              # Snake 类（纯游戏逻辑，含 check_collide 友元）
    ├── snake.cpp            # Snake 实现
    ├── snake_render.h       # SnakeBlock（蛇身单节精灵） + Vector2 工具函数
    ├── snake_anim.h         # SnakeBody（蛇身渲染体） + 动画状态机
    └── snake_state/
        └── animate_manager.h # AnimateState 基类 + AnimateManager（状态机）
```

### 单线程主循环（SceneManager::run）

~~其实抄了不少 Godot/Unity 的作业~~

```
while (!WindowShouldClose()) {
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

由于库限制 *~~设计缺陷？~~*，所有 raylib 调用（InitWindow, LoadTexture, Draw*, BeginDrawing/EndDrawing）必须在同一个线程上，否则有 thread_local 问题。

### Scene 生命周期

| 阶段 | 方法 | 说明 |
|------|------|------|
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
    LogLevel loglevel = LogLevel::Info;   // 日志级别（Debug/Info/Warning/Error）

    bool should_quit = false;

    int  last_score_player1 = 0;          // 玩家 1 最终得分
    int  last_score_player2 = 0;          // 玩家 2 最终得分

    enum class PlayerStatus {
        ALIVE = -1,  // 存活，不显示
        NONE = 0,
        ON_WALL, ON_SELF, ON_PLAYER, STARVED,
    };
    PlayerStatus player_status1 = PlayerStatus::ALIVE;
    PlayerStatus player_status2 = PlayerStatus::ALIVE;

    // 全局结束原因。高位原因不会被低位覆盖（FULL_BOARD > TIMEOUT > MANUAL > DEATH）
    enum class GameOverReason {
        NONE = 0,
        DEATH,      // 玩家死亡导致的结束
        TIMEOUT,
        MANUAL,
        FULL_BOARD, // 棋盘被占满，无法生成苹果
    };
    GameOverReason end_reason = GameOverReason::NONE;

    // 游戏模式
    enum class GameMode { DEATHMATCH, TIMERACE };
    GameMode last_game_mode = GameMode::DEATHMATCH;

    void request_quit();
    bool is_quit_requested();
    void reset();
}
```

结算场景根据 `end_reason` 决定显示：
- `FULL_BOARD` → "You've filled everywhere!!!"
- `TIMEOUT` → "TIME LIMIT REACHED"
- `MANUAL` → "GAME ENDED"
- `DEATH` → 逐玩家显示 `player_status1`/`player_status2`（跳过 `ALIVE`），至多两行

### 日志系统

线程安全的彩色日志，级别过滤 + raylib 日志接管：

| 级别 | 函数 | 颜色 |
|------|------|------|
| Debug | `logd(msg)` | 青 |
| Info | `log(msg)` | 绿 |
| Warning | `logw(msg)` | 黄 |
| Error | `loge(msg)` | 红 |

```cpp
log("Loading config...");
logw("sound not found: " + name);
loge("audio device init failed");
```

- `raylib_log_callback` 接管 raylib 内部日志，按级别映射到对应颜色
- `g_log_mutex` 保护输出，`localtime_r` 线程安全时间戳
- `std::ios::sync_with_stdio(false)` 解除 C++/C 流同步

### 音频系统

采用**共享设备 + 预解码 PCM + 回调混音**架构：

```
AudioManager (单例，Global::audio_manager)
├── ma_device (唯一，48000Hz 立体声 f32)
├── mp: map<name, AudioStreamPlayer>    模板库（不直接播放）
├── current_sound_                     当前独占 sound
└── sfx_pool_                          并发 sfx 克隆池

AudioStreamPlayer
└── AudioLoader (unique_ptr)
      ├── 构造时预解码 MP3 → pcm_buffer_（float32 PCM）
      ├── play() → 重置 cursor_，设状态 Playing
      ├── stop() → 设状态 Stopped，重置 cursor_
      ├── clone() → 从同文件重建 AudioLoader
      └── mix(out, n, outCh, outRate) → 线性插值重采样 + 音量 + 声道转换

回调 (ma_data_callback):
  memset(output, 0)
  → 若 current_sound_ 播放中 → loader->mix(...)
  → 遍历 sfx_pool_ → 每个 loader->mix(...)
```

**API：**

| 方法 | 说明 |
|------|------|
| `play_sound(name)` | 独占播放，停止前一个 sound |
| `play_sfx(name)` | 克隆模板加入并发池，自动清理已停实例 |
| `stop_sound()` | 停止当前 sound |
| `set_volume_all(0-100)` | 设置所有模板音量（线性 0.0–1.0） |
| `init_device()` | 创建并启动共享 ma_device |

**关键细节：**
- 所有 MP3 加载时预解码为 float32 PCM，回调只做 memcpy + 混音
- `play_sfx` 每次 clone 新 AudioLoader 加入池，`cleanup_pool` 移除 `!isPlaying()` 的实例
- `mix()` 内置线性插值重采样：同采样率走快速路径，不同采样率自动转换（44100→48000 等）
- 设备持续运行，无声音时输出静音，永不调用 `ma_device_stop`（避免回调内死锁）

### 配置加载细节

```cpp
const auto &cfg = game_config();
cfg.volume;                          // 音量 0-100（线性）
cfg.allowAcceleration;               // 是否加速（Shift 2倍速）
cfg.toroidalSpace;                   // 环面地图（穿墙）
cfg.allowThroughOthers;              // 是否穿过对方
cfg.speed_factor;                    // 整体速度倍率（最小 0.1）
cfg.increasing_difficulty;           // 难度递增系数（0 = 恒定速度）
cfg.time_match_duration;             // TIMERACE 限时秒数（0 = 无穷，UI 显示 inf）
cfg.reborn_costs;                    // TIMERACE 死亡后从尾部切除的节数
cfg.respawnInAdvance;                // 虚影复活模式（死后随机位置生成半透明虚影）
cfg.deathAnimInterruptThreshold;     // 死亡动画跳过阈值，-1 = 关闭，0+ = 已播放节数超此值时允许按键跳过
```

- 如果发现配置文件缺少字段，则使用默认值补全，程序正常退出时写入完整配置文件
- 如果无法读取/写入配置文件，则在日志中输出警告，但不影响游戏运行
- 如果发现 `snake.cfg` 是文件夹，则会有 *彩蛋* 输出，仍然使用默认配置

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
|------|------|------|
| MOVE → DIE | 蛇死亡 (`apply_death`) | 播放逐节 X 标记 |
| DIE → WAITING | 动画完成 / 按键打断 | 虚影模式部署 ghost |
| WAITING → MOVE | 按键复活 (`handle_respawn`) | 部署真身 |
| MOVE → WAITING | DEATHMATCH 幸存者 | 冻结显示 |

**Ghost 虚影流程**（TIMERACE + `respawnInAdvance`）：
1. 死亡 → 播放死亡动画（原位置 X 标记）
2. 动画完成 → 先从尾部切除 `reborn_costs` 节，更新分数，**再** `generate_ghost()`（随机位置，alpha=100 半透明）
3. 玩家看到虚影位置，按键 → `deploy_from_ghost()` → MOVE（不再重复扣分）

**死亡动画打断**（TIMERACE，`deathAnimInterruptThreshold >= 0`）：
- 设置 `-1` → 关闭打断，必须看完完整死亡动画
- 设置 `0+` → 前 N 节 X 标记正常播放，N 节后按移动键跳过剩余

### 已知缺陷与设计取舍

#### 1. 纹理无引用计数（raylib 上游）

raylib 的 `LoadTextureFromImage()` 每次调用 `rlLoadTexture()` 新建 OpenGL 纹理对象，不做去重也不维护引用计数。当前方案：`Sprite::refresh_texture()` 按需加载，用完即卸，每个 Sprite 独享自己的 texture ID。

#### 2. 音频单线程依赖（raylib 上游）

raylib 的 `Music`/`Sound` 要求主线程每帧调用 `UpdateMusicStream()`。我们的方案：基于 miniaudio 自研 AudioManager，预解码为 PCM + 回调混音，音频线程独立于主线程。

#### 3. `ImageFromImage` 边界条件错误（已向上游提 PR，Merged）

已提交修复 → [raysan5/raylib#5979](https://github.com/raysan5/raylib/pull/5979)

#### 4. `Sprite` 兼职过多（设计取舍）

一个 `Sprite` 同时负责图片加载、GPU 纹理生命周期、变换、精灵表帧动画和绘制提交。本项目规模下拆分反而增加样板代码。

#### 5. 动画状态机与渲染体耦合（设计取舍）

`SnakeMove`/`SnakeDie`/`SnakeWaiting` 直接持有 `SnakeBody*` 裸指针。双人贪吃蛇就 2 个角色，上完整的 ECS 属于大炮打蚊子。

#### 6. 物理帧即渲染帧、无独立碰撞系统（raylib 上游 + 设计取舍）

tick 逻辑和 render 跑在同一个 `update(dt)` 回调里，没有独立的固定时间步物理循环。raylib 的 `thread_local` 限制让分离渲染线程变成不可能的任务。~~吕布骑狗了有一说一~~

## License

本项目源码许可请参见仓库根目录。第三方依赖各自遵循其原有许可证：

- [raylib](https://github.com/raysan5/raylib) — zlib/libpng
- [miniaudio](https://github.com/mackron/miniaudio) — MIT / Public Domain
- [dr_libs](https://github.com/mackron/dr_libs) — MIT / Public Domain
