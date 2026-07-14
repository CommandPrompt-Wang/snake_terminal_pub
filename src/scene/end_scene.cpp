#include "end_scene.h"
#include "global.h"
#include "raylib.h"

#include <cstring>
#include <string>

void EndScene::on_enter() {
    finished_ = false;
    current_option_ = Option::RESTART;

    int score1 = Global::last_score_player1;
    int score2 = Global::last_score_player2;

    die_reason_ = build_die_reason();

    if (score1 > score2) {
        winner_text_ = "PLAYER1 WINS";
        winner_color_ = DARKGREEN;
    } else if (score2 > score1) {
        winner_text_ = "PLAYER2 WINS";
        winner_color_ = DARKBLUE;
    } else {
        winner_text_ = "DRAW";
        winner_color_ = GRAY;
    }
}

void EndScene::on_exit() {
    // nothing to clean up
}

void EndScene::on_inputevent(InputEvent& event) {
    if (!event.is_key_press()) return;

    switch (event.get_key_code()) {
        case KEY_I:
        case KEY_W:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) - 1 + OPTION_COUNT) % OPTION_COUNT);
            event.consume();
            break;
        case KEY_K:
        case KEY_S:
            current_option_ = static_cast<Option>(
                (static_cast<int>(current_option_) + 1) % OPTION_COUNT);
            event.consume();
            break;
        case KEY_ESCAPE:
            finished_ = true;
            next_scene_id_ = SceneId::MENU;
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
}

void EndScene::render() {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // === GAME OVER title ===
    DrawText("GAME OVER",
             screenW / 2 - MeasureText("GAME OVER", 50) / 2,
             screenH / 4 - 30,
             50, Color{230, 41, 55, 255});

    DrawText(die_reason_.c_str(),
             screenW / 2 - MeasureText(die_reason_.c_str(), 20) / 2,
             screenH / 4 + 30,
             20, GRAY);

    // === Score area ===
    const int scoreY = screenH / 2 - 40;

    // PLAYER1 score
    DrawText("PLAYER1",
             screenW / 4 - MeasureText("PLAYER1", 20) / 2,
             scoreY - 30,
             20, DARKGREEN);
    const char* s1 = TextFormat("%d", Global::last_score_player1);
    DrawText(s1,
             screenW / 4 - MeasureText(s1, 30) / 2,
             scoreY,
             30, DARKGREEN);

    // PLAYER2 score
    DrawText("PLAYER2",
             screenW * 3 / 4 - MeasureText("PLAYER2", 20) / 2,
             scoreY - 30,
             20, DARKBLUE);
    const char* s2 = TextFormat("%d", Global::last_score_player2);
    DrawText(s2,
             screenW * 3 / 4 - MeasureText(s2, 30) / 2,
             scoreY,
             30, DARKBLUE);

    // === Winner text ===
    DrawText(winner_text_.c_str(),
             screenW / 2 - MeasureText(winner_text_.c_str(), 28) / 2,
             scoreY + 50,
             28, winner_color_);

    // === Options (centered) ===
    const char* labels[] = {"RESTART", "MENU", "EXIT"};
    const int optFont = 22;
    const int optGap = 65;
    const int optStartY = screenH / 2 + 85;

    for (int i = 0; i < OPTION_COUNT; ++i) {
        int y = optStartY + i * optGap;
        int w = MeasureText(labels[i], optFont) + 40;
        int h = 36;
        int x = screenW / 2 - w / 2;

        if (static_cast<int>(current_option_) == i) {
            DrawRectangle(x, y, w, h, Color{230, 41, 55, 255});
            DrawText(labels[i],
                     screenW / 2 - MeasureText(labels[i], optFont) / 2,
                     y + (h - optFont) / 2,
                     optFont, BLACK);
            DrawText(">",
                     x - 30,
                     y + (h - optFont) / 2,
                     optFont, Color{230, 41, 55, 255});
        } else {
            DrawText(labels[i],
                     screenW / 2 - MeasureText(labels[i], optFont) / 2,
                     y + (h - optFont) / 2,
                     optFont, LIGHTGRAY);
        }
    }
}

bool EndScene::is_finished() const {
    return finished_;
}

int EndScene::get_next_scene_id() const {
    return static_cast<int>(next_scene_id_);
}

const char* EndScene::status_text(Global::PlayerStatus s, int player) {
    const char* p = (player == 1) ? "PLAYER1" : "PLAYER2";
    switch (s) {
        case Global::PlayerStatus::ON_WALL:   return TextFormat("%s HIT THE WALL", p);
        case Global::PlayerStatus::ON_SELF:   return TextFormat("%s HIT ITSELF", p);
        case Global::PlayerStatus::ON_PLAYER: return TextFormat("%s COLLIDED", p);
        case Global::PlayerStatus::STARVED:   return TextFormat("%s STARVED", p);
        default: return "";
    }
}

std::string EndScene::build_die_reason() {
    // 全局原因优先
    if (Global::end_reason == Global::GameOverReason::TIMEOUT)
        return "TIME LIMIT REACHED";
    if (Global::end_reason == Global::GameOverReason::MANUAL)
        return "GAME ENDED";

    // 玩家死亡原因，至多两行
    std::string r;
    auto s1 = Global::player_status1;
    auto s2 = Global::player_status2;
    if (s1 != Global::PlayerStatus::ALIVE) r += status_text(s1, 1);
    if (s2 != Global::PlayerStatus::ALIVE) {
        if (!r.empty()) r += '\n';
        r += status_text(s2, 2);
    }
    return r;
}