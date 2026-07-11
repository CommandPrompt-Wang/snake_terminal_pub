#include "raylib.h"
#include "sprite.h"
#include "draw_process.h"

int main()
{
    InitWindow(800, 450, "snake_terminal");
    SetTargetFPS(60);

    Image img = GenImageColor(64, 64, BLANK);
    ImageDrawRectangle(&img, 0, 0, 32, 32, RED);
    ImageDrawRectangle(&img, 32, 0, 32, 32, GREEN);
    ImageDrawRectangle(&img, 0, 32, 32, 32, BLUE);
    ImageDrawRectangle(&img, 32, 32, 32, 32, ORANGE);

    // Sprite 由外部持有；list 只存指针，运行中可继续改 demo 的资源
    Sprite demo(img);
    UnloadImage(img);
    demo.set_hframes(2);
    demo.set_vframes(2);
    demo.set_pos(Vector2{368, 193});
    demo.set_during_time(1.0f);
    demo.play();

    draw_list_push_back(&demo);

    while (!WindowShouldClose())
    {
        draw_process();
    }

    draw_list_clear();
    CloseWindow();
    return 0;
}
