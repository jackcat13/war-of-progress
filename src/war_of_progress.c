#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

Camera camera = { 0 };
Vector3 cubePosition = { 0 };

static void UpdateDrawFrame(void);

int main()
{
    InitWindow(0, 0, "War of progress");

    camera.position = (Vector3){ 3.0f, 3.0f, 2.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif
    CloseWindow();
    return 0;
}

static void UpdateDrawFrame(void)
{
    UpdateCamera(&camera, CAMERA_ORBITAL);
    BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawFPS(10, 10);

    EndDrawing();
}
