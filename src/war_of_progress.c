#include "raylib.h"
#include <stdio.h>

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
static float ToXIso(int, int);
static float ToYIso(int, int);
static float ToXInvertedIso(int, int);
static float ToYInvertedIso(int, int);
static void InitGame(void);
static void FreeGame(void);

enum EntityType {
  // Units
  VILLAGER,

  // Buildings
  CITY_HALL
};

struct Entity {
  int x;
  int y;
  enum EntityType type;
  int hp;
  int animFramesNumber;
  int animCurrentFrame;
};

enum Scene { MENU, MAIN_GAME };
enum Scene current_scene = MENU;
enum Tile { GRASS };

static Texture2D TileToTexture(enum Tile);
static Texture2D EntityToTexture(enum EntityType);

Camera2D camera = {0};
Texture2D grassTexture;
Texture2D primitiveCityHallTexture;
Texture2D primitiveVillagerTexture;
Vector2 mapSize = {200, 200};
enum Tile **map = NULL;
struct Entity *entities = NULL;
int entitiesSize;

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
  FreeGame();
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
    InitGame();
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

  // Draw Map

  int i, j;
  int iMin = ToXInvertedIso(camera.target.x, camera.target.y) - 50;
  int jMin = ToYInvertedIso(camera.target.x, camera.target.y) - 50;
  int iMax = iMin + 100;
  int jMax = jMin + 100;
  if (iMin < 0) {
    iMin = 0;
  }
  if (jMin < 0) {
    jMin = 0;
  }
  if (jMax > mapSize.y) {
    jMax = mapSize.y;
  }
  if (iMax > mapSize.x) {
    iMax = mapSize.x;
  }
  for (j = jMin; j < jMax; j++) {
    for (i = iMin; i < iMax; i++) {
      float x = ToXIso(i, j);
      float y = ToYIso(i, j);
      Texture2D texture = TileToTexture(map[i][j]);
      DrawTexture(texture, x, y, WHITE);
    }
  }

  // Draw entities
  for (i = 0; i < entitiesSize; i++) {
    struct Entity *entity = &entities[i];
    Texture2D texture = EntityToTexture(entity->type);
    int x = entity->x;
    int y = entity->y;
    int animWidth = texture.width / entity->animFramesNumber;
    int animOffset = entity->animCurrentFrame * animWidth;
    DrawTextureRec(texture,
                   (Rectangle){animOffset, 0, animWidth, texture.height},
                   (Vector2){x, y}, WHITE);
    entity->animCurrentFrame++;
    if (entity->animCurrentFrame > entity->animFramesNumber) {
      entity->animCurrentFrame = 1;
    }
  }

  EndMode2D();

  DrawFPS(screenWidth - MARGIN, MARGIN);
}

// INITS

static void InitCamera(void) {
  float screenWidth = GetScreenWidth();
  float screenHeight = GetScreenHeight();
  float map_center_x = ToXIso(mapSize.x / 2, mapSize.y / 2);
  float map_center_y = ToYIso(mapSize.x / 2, mapSize.y / 2);
  camera.target = (Vector2){map_center_x, map_center_y};
  camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 0.3f;
}

static void InitTextures(void) {
  grassTexture = LoadTexture("assets/map/grass.png");
  primitiveCityHallTexture =
      LoadTexture("assets/primitive/buildings/cityHall.png");
  primitiveVillagerTexture = LoadTexture("assets/primitive/units/villager.png");
}

void InitMap(void) {
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

void InitEntities(void) {
  entitiesSize = 4;
  entities = (struct Entity *)malloc(entitiesSize * sizeof(struct Entity));

  int mapCenterX = camera.target.x;
  int mapCenterY = camera.target.y;

  // CITY_HALL
  entities[0] = (struct Entity){mapCenterX, mapCenterY, CITY_HALL, 3000, 7, 1};

  // VILLAGERS
  entities[1] =
      (struct Entity){mapCenterX - 400, mapCenterY, VILLAGER, 100, 1, 1};
  entities[2] =
      (struct Entity){mapCenterX, mapCenterY - 400, VILLAGER, 100, 1, 1};
  entities[3] =
      (struct Entity){mapCenterX, mapCenterY + 1100, VILLAGER, 100, 1, 1};
}

static void InitGame(void) {
  InitMap();
  InitEntities();
}

static void FreeGame(void) {
  free(entities);
  entities = NULL;
  free(map);
  map = NULL;
}

static Texture2D TileToTexture(enum Tile tile) {
  switch (tile) {
  case GRASS:
    return grassTexture;
    break;
  }
}

static Texture2D EntityToTexture(enum EntityType type) { // TODO: support ages
  switch (type) {
  case CITY_HALL:
    return primitiveCityHallTexture;
    break;
  case VILLAGER:
    return primitiveVillagerTexture;
    break;
  }
}

// ISOMETRIC HELPERS

static float ToXIso(int x, int y) {
  return (float)(x - y) * (grassTexture.width / 2.0f);
}

static float ToYIso(int x, int y) {
  return (float)(x + y) * (grassTexture.height / 4.0f);
}

static float ToXInvertedIso(int iso_x, int iso_y) {
  return (float)((iso_x / (grassTexture.width / 2.0f)) +
                 (iso_y / (grassTexture.height / 4.0f))) /
         2.0f;
}

static float ToYInvertedIso(int iso_x, int iso_y) {
  return (float)((iso_y / (grassTexture.height / 4.0f)) -
                 (iso_x / (grassTexture.width / 2.0f))) /
         2.0f;
}

// Mouse interactions

const int SCROLL_MOVE = 80;

static void CheckMouseScroll(void) {
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();
  Vector2 mouse_position = GetMousePosition();
  if (mouse_position.x >= screen_width - MARGIN) {
    camera.target.x += SCROLL_MOVE;
  } else if (mouse_position.x <= MARGIN) {
    camera.target.x -= SCROLL_MOVE;
  }
  if (mouse_position.y >= screen_height - MARGIN) {
    camera.target.y += SCROLL_MOVE;
  } else if (mouse_position.y <= MARGIN) {
    camera.target.y -= SCROLL_MOVE;
  }
}

static void CheckMouseZoom(void) {
  float wheelDelta = GetMouseWheelMove() / 10;
  if (camera.zoom - wheelDelta > 0.1f && camera.zoom - wheelDelta < 3.0f) {
    camera.zoom -= wheelDelta;
  }
}
