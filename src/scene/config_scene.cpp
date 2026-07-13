#include "config_scene.h"
#include "config/config.h"
#include "raylib.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

void ConfigScene::on_enter() {
    finished_ = false;
    current_option_ = Option::ALLOW_ACCELERATION;
}

void ConfigScene::on_exit() {
    // nothing to clean up
}

void ConfigScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    Config& cfg = game_config();

    switch (event.get_key_code()) {
        case KEY_UP:
        case KEY_W:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) - 1 + OPTION_COUNT) % OPTION_COUNT);
            event.consume();
            break;

        case KEY_DOWN:
        case KEY_S:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) + 1) % OPTION_COUNT);
            event.consume();
            break;

        case KEY_LEFT:
        case KEY_A:
            switch (current_option_) {
                case Option::ALLOW_ACCELERATION:
                    cfg.allowAcceleration = !cfg.allowAcceleration;
                    break;
                case Option::TOROIDAL_SPACE:
                    cfg.toroidalSpace = !cfg.toroidalSpace;
                    break;
                case Option::ALLOW_THROUGH_OTHERS:
                    cfg.allowThroughOthers = !cfg.allowThroughOthers;
                    break;
                case Option::SPEED_FACTOR:
                    cfg.speed_factor = std::max(0.1f, cfg.speed_factor - 0.1f);
                    break;
                case Option::INCREASING_DIFFICULTY:
                    cfg.increasing_difficulty = std::max(0.0f, cfg.increasing_difficulty - 0.1f);
                    break;
                case Option::BACK:
                    break;
            }
            event.consume();
            break;

        case KEY_RIGHT:
        case KEY_D:
            switch (current_option_) {
                case Option::ALLOW_ACCELERATION:
                    cfg.allowAcceleration = !cfg.allowAcceleration;
                    break;
                case Option::TOROIDAL_SPACE:
                    cfg.toroidalSpace = !cfg.toroidalSpace;
                    break;
                case Option::ALLOW_THROUGH_OTHERS:
                    cfg.allowThroughOthers = !cfg.allowThroughOthers;
                    break;
                case Option::SPEED_FACTOR:
                    cfg.speed_factor += 0.1f;
                    break;
                case Option::INCREASING_DIFFICULTY:
                    cfg.increasing_difficulty += 0.1f;
                    break;
                case Option::BACK:
                    break;
            }
            event.consume();
            break;

        case KEY_ENTER:
        case KEY_SPACE:
            if (current_option_ == Option::BACK) {
                finished_ = true;
                next_scene_id_ = static_cast<int>(SceneId::MENU);
            }
            event.consume();
            break;

        case KEY_ESCAPE:
        case KEY_SLASH:
            finished_ = true;
            next_scene_id_ = static_cast<int>(SceneId::MENU);
            event.consume();
            break;

        default:
            break;
    }
}

void ConfigScene::update(float /*delta_time*/) {
}

void ConfigScene::render() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // === Title ===
    DrawText("CONFIG",
             screenW / 2 - MeasureText("CONFIG", 53) / 2,
             screenH / 6 - 25,
             53, DARKGREEN);

    Config& cfg = game_config();

    // === Option labels ===
    const char* labels[] = {
        "Allow Acceleration",
        "Toroidal Space",
        "Allow Through Other Player",
        "Speed Factor",
        "Increasing Difficulty",
        "BACK",
    };

    const int optFont = 25;
    const int optGap  = 62;
    const int optStartY = screenH / 3;

    for (int i = 0; i < OPTION_COUNT; ++i) {
        int y = optStartY + i * optGap;
        bool isBack = (i == static_cast<int>(Option::BACK));
        bool selected = (static_cast<int>(current_option_) == i);

        // Build value string
        const char* valueStr = nullptr;
        char floatBuf[16]{};

        if (!isBack) {
            switch (static_cast<Option>(i)) {
                case Option::ALLOW_ACCELERATION:
                    valueStr = cfg.allowAcceleration ? "ON" : "OFF";
                    break;
                case Option::TOROIDAL_SPACE:
                    valueStr = cfg.toroidalSpace ? "ON" : "OFF";
                    break;
                case Option::ALLOW_THROUGH_OTHERS:
                    valueStr = cfg.allowThroughOthers ? "ON" : "OFF";
                    break;
                case Option::SPEED_FACTOR:
                    std::snprintf(floatBuf, sizeof(floatBuf), "%.1f", cfg.speed_factor);
                    valueStr = floatBuf;
                    break;
                case Option::INCREASING_DIFFICULTY:
                    std::snprintf(floatBuf, sizeof(floatBuf), "%.1f", cfg.increasing_difficulty);
                    valueStr = floatBuf;
                    break;
                default:
                    break;
            }
        }

        if (selected) {
            // ── Highlighted option ──
            char lineBuf[96];
            if (isBack) {
                std::snprintf(lineBuf, sizeof(lineBuf), "%s", labels[i]);
            } else {
                std::snprintf(lineBuf, sizeof(lineBuf), "< %s : %s >", labels[i], valueStr);
            }

            int w = MeasureText(lineBuf, optFont) + 40;
            int h = 40;
            int x = screenW / 2 - w / 2;
            DrawRectangle(x, y, w, h, Color{230, 41, 55, 255});
            DrawText(lineBuf,
                     screenW / 2 - MeasureText(lineBuf, optFont) / 2,
                     y + (h - optFont) / 2,
                     optFont, BLACK);
        } else {
            // ── Normal option ──
            char lineBuf[80];
            if (isBack) {
                std::snprintf(lineBuf, sizeof(lineBuf), "%s", labels[i]);
            } else {
                std::snprintf(lineBuf, sizeof(lineBuf), "%s : %s", labels[i], valueStr);
            }
            DrawText(lineBuf,
                     screenW / 2 - MeasureText(lineBuf, optFont) / 2,
                     y + (40 - optFont) / 2,
                     optFont, LIGHTGRAY);
        }
    }

    // === Hint at bottom ===
    DrawText("Arrow keys to navigate / adjust    ESC to return",
             screenW / 2 - MeasureText("Arrow keys to navigate / adjust    ESC to return", 18) / 2,
             screenH - 40,
             18, GRAY);
}

bool ConfigScene::is_finished() const {
    return finished_;
}

int ConfigScene::get_next_scene_id() const {
    return next_scene_id_;
}
