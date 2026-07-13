#include "end_scene.h"
#include "global.h"
#include "raylib.h"

#include <cstring>

void EndScene::on_enter() {
    finished_ = false;
    current_option_ = Option::RESTART;

    int score1 = Global::last_score_player1;
    int score2 = Global::last_score_player2;

    if (score1 > score2) {
        std::strncpy(winner_text_, "PLAYER1 WINS", sizeof(winner_text_) - 1);
        winner_color_ = DARKGREEN;
    } else if (score2 > score1) {
        std::strncpy(winner_text_, "PLAYER2 WINS", sizeof(winner_text_) - 1);
        winner_color_ = DARKBLUE;
    } else {
        std::strncpy(winner_text_, "DRAW", sizeof(winner_text_) - 1);
        winner_color_ = GRAY;
    }
    winner_text_[sizeof(winner_text_) - 1] = '\0';
}

void EndScene::on_exit() {
    // nothing to clean up
}

void EndScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_UP:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) - 1 + OPTION_COUNT) % OPTION_COUNT);
            event.consume();
            break;
        case KEY_DOWN:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) + 1) % OPTION_COUNT);
            event.consume();
            break;
        case KEY_ENTER:
        case KEY_SPACE:
            switch (current_option_) {
                case Option::RESTART:
                    finished_ = true;
                    next_scene_id_ = SceneId::GAME;
                    break;
                case Option::MENU:
                    finished_ = true;
                    next_scene_id_ = SceneId::MENU;
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

void EndScene::update(float /*delta_time*/) {
    // Direct raylib key check as fallback
    if (IsKeyPressed(KEY_UP)) {
        current_option_ = static_cast<Option>(
            (static_cast<int>(current_option_) - 1 + OPTION_COUNT) % OPTION_COUNT);
    }
    if (IsKeyPressed(KEY_DOWN)) {
        current_option_ = static_cast<Option>(
            (static_cast<int>(current_option_) + 1) % OPTION_COUNT);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (current_option_) {
            case Option::RESTART:
                finished_ = true;
                next_scene_id_ = SceneId::GAME;
                break;
            case Option::MENU:
                finished_ = true;
                next_scene_id_ = SceneId::MENU;
                break;
            case Option::QUIT:
                Global::request_quit();
                break;
        }
    }
}

void EndScene::render() {
    // === GAME OVER title ===
    // UI: rect (170, 80, 301, 71), font 26, color #e62937, centered
    // Center X = 170 + 301/2 = 320
    const int titleCenterX = 170 + 301 / 2;
    const int titleY = 80 + (71 - 26) / 2;
    DrawText("GAME OVER",
             titleCenterX - MeasureText("GAME OVER", 26) / 2,
             titleY,
             26, Color{230, 41, 55, 255});

    // === Player labels ===
    // PLAYER1: rect (70, 200, 150, 40), font 16, centered
    const int p1LabelCenterX = 70 + 150 / 2;
    DrawText("PLAYER1",
             p1LabelCenterX - MeasureText("PLAYER1", 16) / 2,
             200 + (40 - 16) / 2,
             16, DARKGREEN);

    // PLAYER2: rect (430, 200, 150, 40), font 16, centered
    const int p2LabelCenterX = 430 + 150 / 2;
    DrawText("PLAYER2",
             p2LabelCenterX - MeasureText("PLAYER2", 16) / 2,
             200 + (40 - 16) / 2,
             16, DARKBLUE);

    // === Scores ===
    // Score1: rect (120, 250, 54, 31), font 14, centered
    const char* score1_str = TextFormat("%d", Global::last_score_player1);
    const int s1CenterX = 120 + 54 / 2;
    DrawText(score1_str,
             s1CenterX - MeasureText(score1_str, 14) / 2,
             250 + (31 - 14) / 2,
             14, DARKGREEN);

    // Score2: rect (480, 250, 54, 31), font 14, centered
    const char* score2_str = TextFormat("%d", Global::last_score_player2);
    const int s2CenterX = 480 + 54 / 2;
    DrawText(score2_str,
             s2CenterX - MeasureText(score2_str, 14) / 2,
             250 + (31 - 14) / 2,
             14, DARKBLUE);

    // === Winner text ===
    // rect (240, 310, 150, 40), font 16, centered
    const int winnerCenterX = 240 + 150 / 2;
    DrawText(winner_text_,
             winnerCenterX - MeasureText(winner_text_, 16) / 2,
             310 + (40 - 16) / 2,
             16, winner_color_);

    // === Menu options ===
    struct MenuItem {
        const char* label;
        int x, y, w, h;
    };
    const MenuItem items[] = {
        {"RESTART", 180, 420, 151, 41},
        {"MENU",    180, 500, 151, 41},
        {"EXIT",    180, 580, 151, 41},
    };

    for (int i = 0; i < OPTION_COUNT; ++i) {
        const auto& item = items[i];
        Color textColor = LIGHTGRAY;

        if (static_cast<int>(current_option_) == i) {
            // Selected option: draw a highlight background
            textColor = BLACK;
            DrawRectangle(item.x, item.y, item.w, item.h, Color{230, 41, 55, 255});

            // Draw ">" pointer to the left of the selected option
            int pointerX = item.x - 30;
            int pointerY = item.y + (item.h - 16) / 2;
            DrawText(">", pointerX, pointerY, 16, Color{230, 41, 55, 255});
        }

        int textX = item.x + (item.w - MeasureText(item.label, 16)) / 2;
        int textY = item.y + (item.h - 16) / 2;
        DrawText(item.label, textX, textY, 16, textColor);
    }
}

bool EndScene::is_finished() const {
    return finished_;
}

int EndScene::get_next_scene_id() const {
    return static_cast<int>(next_scene_id_);
}

