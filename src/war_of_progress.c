#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static void InitTextures(void);
static void FreeTextures(void);
static void UpdateDrawFrame(void);
static void RenderMenu(void);
static void RenderMainGame(void);
static void InitCamera(void);
static void CheckScroll(void);
static void CheckMouseZoom(void);
static void CheckSelect(void);
static float ToXIso(int, int);
static float ToYIso(int, int);
static float ToXInvertedIso(int, int);
static float ToYInvertedIso(int, int);
static void InitGame(void);
static void FreeGame(void);
static void DrawTopHud(void);

enum EntityType {
  // Units
  VILLAGER,

  // Buildings
  CITY_HALL
};

struct Entity {
  Vector2 position;
  enum EntityType type;
  int hp;
  int animFramesNumber;
  int animCurrentFrame;
  bool isSelected;
};

struct Resources {
  int wood;
  int stone;
  int gold;
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
int entitiesSize = 0;
struct Resources resources;

const int GAME_FONT_SIZE = 20;

int main() {
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(0, 0, "War of progress");
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
  FreeTextures();
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
    CheckScroll();
    CheckMouseZoom();
    CheckSelect();
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
    int x = entity->position.x;
    int y = entity->position.y;
    int animWidth = texture.width / entity->animFramesNumber;
    int animOffset = entity->animCurrentFrame * animWidth;
    Color textureColor = WHITE;
    if (entity->isSelected) {
      textureColor = (Color) {66, 245, 102, 220}; 
    }
    DrawTextureRec(texture,
                   (Rectangle){animOffset, 0, animWidth, texture.height},
                   (Vector2){x, y}, textureColor);
    entity->animCurrentFrame++;
    if (entity->animCurrentFrame > entity->animFramesNumber) {
      entity->animCurrentFrame = 1;
    }
  }

  EndMode2D();

  DrawTopHud();
}

static void DrawTopHud(void) {
  int screenWidth = GetScreenWidth();
  DrawRectangle(0, 0, screenWidth, MARGIN * 2, BLACK);
  const char *resourcesText =
      TextFormat("Wood : %i - Stone : %i - Gold : %i", resources.wood,
                 resources.stone, resources.gold);
  DrawText(resourcesText, MARGIN, MARGIN, 20, WHITE);
  DrawFPS(screenWidth - MARGIN - MeasureText("120 FPS", GAME_FONT_SIZE),
          MARGIN);
}

// SELECTED ENTITIES HELPERS

static void AddToSelectedEntities(struct Entity *entity) {
  printf("Add to selected\n");
  entity->isSelected = true;
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
  // MAP TILES
  grassTexture = LoadTexture("assets/map/grass.png");

  // Buildings
  primitiveCityHallTexture =
      LoadTexture("assets/primitive/buildings/cityHall.png");

  // Units
  primitiveVillagerTexture = LoadTexture("assets/primitive/units/villager.png");
}

static void FreeTextures(void) {
  UnloadTexture(grassTexture);
  UnloadTexture(primitiveCityHallTexture);
  UnloadTexture(primitiveVillagerTexture);
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

void InitResources(void) {
  resources.wood = 50;
  resources.stone = 50;
  resources.gold = 0;
}

static void InitGame(void) {
  InitMap();
  InitEntities();
  InitResources();
}

static void FreeEntities(void) {
  if (entities) {
    free(entities);
    entities = NULL;
    entitiesSize = 0;
  }
}

static void FreeSelectedEntities(void) {
  for (int i = 0; i < entitiesSize; i++) {
    entities[i].isSelected = false;
  }
}

static void FreeMap(void) {
  if (map) {
    for (int i = 0; i < mapSize.y; i++) {
      if (map[i]) {
        free(map[i]);
      }
    }
    free(map);
    map = NULL;
  }
}

static void FreeGame(void) {
  FreeEntities();
  FreeSelectedEntities();
  FreeMap();
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

// COLLISIONS HELPERS

static bool IsRightScreenHit(int x) { return x >= GetScreenWidth() - MARGIN; }

static bool IsLeftScreenHit(int x) { return x <= MARGIN; }

static bool IsBottomScreenHit(int y) { return y >= GetScreenHeight() - MARGIN; }

static bool IsTopScreenHit(int y) { return y <= MARGIN; }

static bool IsHit(Vector2 testPosition, Vector2 targetPosition,
                  Vector2 targetSize) {
  printf("\n%f %f %f %f %f %f\n", testPosition.x, testPosition.y,
         targetPosition.x, targetPosition.y, targetSize.x, targetSize.y);
  return testPosition.x >= targetPosition.x &&
         testPosition.x <= (targetPosition.x + targetSize.x) &&
         testPosition.y >= targetPosition.y &&
         testPosition.y <= (targetPosition.y + targetSize.y);
}

// Mouse or Keyboard interactions

const int SCROLL_MOVE = 80;

static void CheckScroll(void) {
  Vector2 mouse_position = GetMousePosition();
  if (IsKeyDown(KEY_RIGHT)) {
    camera.target.x += SCROLL_MOVE;
  } else if (IsKeyDown(KEY_LEFT)) {
    camera.target.x -= SCROLL_MOVE;
  }
  if (IsKeyDown(KEY_DOWN)) {
    camera.target.y += SCROLL_MOVE;
  } else if (IsKeyDown(KEY_UP)) {
    camera.target.y -= SCROLL_MOVE;
  }
  if (IsRightScreenHit(mouse_position.x)) {
    camera.target.x += SCROLL_MOVE;
  } else if (IsLeftScreenHit(mouse_position.x)) {
    camera.target.x -= SCROLL_MOVE;
  }
  if (IsBottomScreenHit(mouse_position.y)) {
    camera.target.y += SCROLL_MOVE;
  } else if (IsTopScreenHit(mouse_position.y)) {
    camera.target.y -= SCROLL_MOVE;
  }
}

static void CheckMouseZoom(void) {
  float wheelDelta = GetMouseWheelMove() / 10;
  if (camera.zoom - wheelDelta > 0.1f && camera.zoom - wheelDelta < 3.0f) {
    camera.zoom -= wheelDelta;
  }
}

static void CheckSelect(void) {
  Vector2 mousePosition = GetMousePosition();
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    return;
  FreeSelectedEntities();
  for (int i = 0; i < entitiesSize; i++) {
    struct Entity *entity = &entities[i];
    Texture2D entityTexture = EntityToTexture(entity->type);
    int entityWidth = entityTexture.width / (float)entity->animFramesNumber;
    int entityHeight = entityTexture.height;
    Vector2 entitySize =
        (Vector2){entityWidth, entityHeight};
    Vector2 mousePositionInWorld = GetScreenToWorld2D(mousePosition, camera);
    if (IsHit(mousePositionInWorld, entity->position, entitySize)) {
      printf("Select checked\n");
      AddToSelectedEntities(entity);
      return;
    }
  }
}
