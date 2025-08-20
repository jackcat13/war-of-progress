#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static void InitTextures(void);
static void UpdateDrawFrame(void);
static void RenderMenu(void);
static void RenderMainGame(void);
static void InitCamera(void);
static void CheckMouseScroll(void);
static float ToXIso(int, int, int, int);
static float ToYIso(int, int, int, int);

enum Scene { MENU, MAIN_GAME };
enum Scene current_scene = MENU;

Camera2D camera = {0};
Texture2D grassTexture;

int main() {
  InitWindow(0, 0, "War of progress");
  ToggleFullscreen();
  GuiLoadStyleDefault();
  InitTextures();

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }
#endif
  CloseWindow();
  return 0;
}

static void UpdateDrawFrame(void) {
  BeginDrawing();

  switch (current_scene) {
  case MENU:
    RenderMenu();
    break;
  case MAIN_GAME:
    CheckMouseScroll();
    RenderMainGame();
    break;
  }

  EndDrawing();
}

// SCENES

static void RenderMenu(void) {
  ClearBackground(BLACK);
  if (GuiButton((Rectangle){24, 24, 120, 30}, "Start game")) {
    InitCamera();
    current_scene = MAIN_GAME;
  }
}

const Color BACKGROUND = BLACK;
const int MARGIN = 20;

static void RenderMainGame(void) {
  float screenWidth = GetScreenWidth();
  float screenHeight = GetScreenHeight();

  BeginMode2D(camera);

  ClearBackground(BACKGROUND);
  int i;
  int j;
  int width = grassTexture.width;
  int height = grassTexture.height;
  for (i = 1; i < 10; i++) {
    for (j = 1; j < 10; j++) {
      float x = ToXIso(i, j, width, height);
      float y = ToYIso(i, j, width, height);
      DrawTexture(grassTexture, ToXIso(i, j, width, height),
                  ToYIso(i, j, width, height), WHITE);
      printf("%f, %f\n", x, y);
    }
  }

  EndMode2D();

  DrawFPS(screenWidth - MARGIN, MARGIN);
}

// INITS

static void InitCamera(void) {
  float screenWidth = GetScreenWidth();
  float screenHeight = GetScreenHeight();
  camera.target = (Vector2){0.0f, 0.0f};
  camera.offset = (Vector2){ screenWidth/2.0f, 0.0f };
  camera.rotation = 0.0f;
  camera.zoom = 0.3f;
}

static void InitTextures(void) {
  grassTexture = LoadTexture("assets/map/grass.png");
}

// ISOMETRIC HELPERS

static float ToXIso(int x, int y, int width, int height) {
  return (float)(x - y) * width / 2;
}

static float ToYIso(int x, int y, int width, int height) {
  return (float)(x + y) * height / 2;
}

// Mouse interactions

const int SCROLL_MOVE = 80;

static void CheckMouseScroll(void) {
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();
  Vector2 mouse_position = GetMousePosition();
  if (mouse_position.x >= screen_width) {
    camera.target.x += SCROLL_MOVE;
  }
  else if (mouse_position.x <= 0) {
    camera.target.x -= SCROLL_MOVE;
  }
  if (mouse_position.y >= screen_height) {
    camera.target.y += SCROLL_MOVE;
  }
  else if (mouse_position.y <= 0) {
    camera.target.y -= SCROLL_MOVE;
  }
}
