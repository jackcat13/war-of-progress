#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static void UpdateDrawFrame(void);
static void RenderMenu(void);
static void RenderMainGame(void);

enum Scene { MENU, MAIN_GAME };
enum Scene current_scene = MENU;

int main() {
  InitWindow(0, 0, "War of progress");
  ToggleFullscreen();
  GuiLoadStyleDefault();

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
    RenderMenu(); break;
  case MAIN_GAME:
    RenderMainGame(); break;
  }

  EndDrawing();
}

static void RenderMenu(void) {
  ClearBackground(BLACK);
  if (GuiButton((Rectangle){24, 24, 120, 30}, "Start game")) {
    current_scene = MAIN_GAME;
  }
}

const int MARGIN = 20;

static void RenderMainGame(void) {
  float screen_width = GetScreenWidth();
  float screen_height = GetScreenHeight();
  ClearBackground(BLACK);
  DrawFPS(screen_width - MARGIN, MARGIN);
}
