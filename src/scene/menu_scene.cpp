#include "scene/menu_scene.h"
#include "global.h"
#include "raylib.h"

void MenuScene::on_enter() {
    logd("menu scene enter");
    finished_ = false;
    current_option_ = Option::START_DEATHMATCH;
    next_scene_id_ = static_cast<int>(SceneId::GAME);
}

void MenuScene::on_exit() {
    // nothing to clean up
}

void MenuScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_I:
        case KEY_W:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) - 1 + OPTION_COUNT) % OPTION_COUNT);
            Global::audio_manager.play_sfx("ui.index_switch");
            event.consume();
            break;

        case KEY_K:
        case KEY_S:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) + 1) % OPTION_COUNT);
            Global::audio_manager.play_sfx("ui.index_switch");
            event.consume();
            break;

        case KEY_ESCAPE:
            // Global::audio_manager.play_sfx("ui.enter");
            Global::request_quit();
            event.consume();
            break;

        case KEY_ENTER:
        case KEY_SPACE:
            switch (current_option_) {
                case Option::START_DEATHMATCH:
                    Global::audio_manager.play_sfx("ui.enter");
                    finished_ = true;
                    Global::last_game_mode = Global::GameMode::DEATHMATCH;
                    next_scene_id_ = static_cast<int>(SceneId::GAME);
                    break;
                case Option::START_TIMERACE:
                    Global::audio_manager.play_sfx("ui.enter");
                    finished_ = true;
                    Global::last_game_mode = Global::GameMode::TIMERACE;
                    next_scene_id_ = static_cast<int>(SceneId::GAME);
                    break;
                case Option::CONFIG:
                    Global::audio_manager.play_sfx("ui.enter");
                    finished_ = true;
                    next_scene_id_ = static_cast<int>(SceneId::CONFIG);
                    break;
                case Option::QUIT:
                    Global::request_quit();
                    break;
            }
            event.consume();
            break;

        default:
            break;
    }
}

void MenuScene::on_quitevent(QuitEvent& event) {
    Global::request_quit();
    event.consume();
}

void MenuScene::update(float /*dt*/) {
}

void MenuScene::render() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // === Title ===
    DrawText("SNAKE TERMINAL",
             screenW / 2 - MeasureText("SNAKE TERMINAL", 53) / 2,
             screenH / 3 - 30,
             53, DARKGREEN);

    // === Options ===
    const char* labels[] = {"DEATH MATCH", "TIME RACE", "CONFIG", "QUIT"};
    const int optFont = 25;
    const int optGap  = 62;
    const int optStartY = screenH / 2;

    for (int i = 0; i < OPTION_COUNT; ++i) {
        int y = optStartY + i * optGap;
        bool selected = (static_cast<int>(current_option_) == i);

        if (selected) {
            int w = MeasureText(labels[i], optFont) + 40;
            int h = 40;
            int x = screenW / 2 - w / 2;
            DrawRectangle(x, y, w, h, Color{230, 41, 55, 255});
            DrawText(labels[i],
                     screenW / 2 - MeasureText(labels[i], optFont) / 2,
                     y + (h - optFont) / 2,
                     optFont, BLACK);
        } else {
            DrawText(labels[i],
                     screenW / 2 - MeasureText(labels[i], optFont) / 2,
                     y + (40 - optFont) / 2,
                     optFont, LIGHTGRAY);
        }
    }

    // === Hint at bottom ===
    DrawText("Arrow keys to navigate    Enter to confirm",
             screenW / 2 - MeasureText("Arrow keys to navigate    Enter to confirm", 18) / 2,
             screenH - 40,
             18, GRAY);
}
