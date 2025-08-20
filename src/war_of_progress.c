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
static void CheckMouseZoom(void);
static float ToXIso(int, int, int, int);
static float ToYIso(int, int, int, int);
static void InitMap(void);

enum Scene { MENU, MAIN_GAME };
enum Scene current_scene = MENU;
enum Tile { GRASS };

static Texture2D TileToTexture(enum Tile);

Camera2D camera = {0};
Texture2D grassTexture;
Vector2 mapSize = {200, 200};
enum Tile **map;

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
    CheckMouseZoom();
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
    InitMap();
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
  for (i = 0; i < mapSize.x; i++) {
    for (j = 0; j < mapSize.y; j++) {
      float x = ToXIso(i, j, width, height);
      float y = ToYIso(i, j, width, height);
      Texture2D texture = TileToTexture(map[i][j]);
      DrawTexture(texture, ToXIso(i, j, width, height),
                  ToYIso(i, j, width, height), WHITE);
    }
  }

  EndMode2D();

  DrawFPS(screenWidth - MARGIN, MARGIN);
}

// INITS

static void InitCamera(void) {
  float screenWidth = GetScreenWidth();
  float screenHeight = GetScreenHeight();
  float map_center_x = ToXIso(mapSize.x / 2, mapSize.y / 2, grassTexture.width,
                              grassTexture.height);
  float map_center_y = ToYIso(mapSize.x / 2, mapSize.y / 2, grassTexture.width,
                              grassTexture.height);
  camera.target = (Vector2){map_center_x, map_center_y};
  camera.offset = (Vector2){screenWidth / 2.0f, 0.0f};
  camera.rotation = 0.0f;
  camera.zoom = 0.3f;
}

static void InitTextures(void) {
  grassTexture = LoadTexture("assets/map/grass.png");
}

static void InitMap(void) {
  int i, j;
  map = (enum Tile **)malloc(mapSize.y * sizeof(enum Tile *));
  for (i = 0; i < mapSize.y; i++) {
    map[i] = (enum Tile *)malloc(mapSize.x * sizeof(enum Tile));
  }
  for (i = 0; i < mapSize.y; i++) {
    for (j = 0; j < mapSize.x; j++) {
      map[i][j] = GRASS;
    }
  }
}

static Texture2D TileToTexture(enum Tile tile) {
  switch (tile) {
  case GRASS:
    return grassTexture;
    break;
  }
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
  } else if (mouse_position.x <= 0) {
    camera.target.x -= SCROLL_MOVE;
  }
  if (mouse_position.y >= screen_height) {
    camera.target.y += SCROLL_MOVE;
  } else if (mouse_position.y <= 0) {
    camera.target.y -= SCROLL_MOVE;
  }
}

static void CheckMouseZoom(void) {
  float wheelDelta = GetMouseWheelMove() / 10;
  if (camera.zoom - wheelDelta > 0.0f && camera.zoom - wheelDelta < 3.0f) {
    camera.zoom -= wheelDelta;
  }
}
