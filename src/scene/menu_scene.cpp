#include "scene/menu_scene.h"
#include "raylib.h"
#include <iostream>

void MenuScene::on_enter() {
    finished_ = false;
}

void MenuScene::on_exit() {
    // nothing to clean up
}

void MenuScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_SPACE:
            finished_ = true;
            event.consume();
            break;
        case KEY_Q:
        // case KEY_ESCAPE:
            Global::request_quit();
            event.consume();
            break;
        default:
            break;
    }
}

void MenuScene::on_quitevent(QuitEvent& event) {
    // Default quit handling — SceneManager will check is_quit_requested
    Global::request_quit();
    event.consume();
}

void MenuScene::update(float /*dt*/) {
    // Direct raylib key check to verify event loop is working
    if (IsKeyPressed(KEY_SPACE)) {
        finished_ = true;
    }
    if (IsKeyPressed(KEY_Q)) {
        Global::request_quit();
    }
}

void MenuScene::render() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawText("SNAKE TERMINAL",
             screenW / 2 - MeasureText("SNAKE TERMINAL", 60) / 2,
             screenH / 3 - 30,
             60, DARKGREEN);

    DrawText("Press SPACE to Start",
             screenW / 2 - MeasureText("Press SPACE to Start", 30) / 2,
             screenH / 2,
             30, GRAY);

    DrawText("WASD / Arrow Keys to Move",
             screenW / 2 - MeasureText("WASD / Arrow Keys to Move", 20) / 2,
             screenH / 2 + 50,
             20, LIGHTGRAY);

    DrawText("Press Q to Quit",
             screenW / 2 - MeasureText("Press Q to Quit", 20) / 2,
             screenH / 2 + 80,
             20, LIGHTGRAY);
}
