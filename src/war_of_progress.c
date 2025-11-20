#include "perlin.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

const int BASE_POPULATION_MAX = 5;
const int SHELTER_POPULATION_NUMBER = 5;

static void InitTextures(void);
static void FreeTextures(void);
static void UpdateDrawFrame(RenderTexture2D);
static void RenderMenu(void);
static void ProcessMovements(void);
static void RenderMainGame(void);
static void InitCamera(void);
static void CheckScroll(Camera2D *);
static void CheckMouseZoom(Camera2D *);
static void CheckSelect(Camera2D *);
static void CheckBuilding(Camera2D *);
static void CheckMovement(Camera2D *);
static void CheckInputs();
static float ToXIso(int, int);
static float ToYIso(int, int);
static float ToXInvertedIso(int, int);
static float ToYInvertedIso(int, int);
static void InitGame(void);
static void FreeGame(void);
static void DrawHelpWindow(int, int);
static void DrawTopHud(void);
static int GetPopulation(void);
static int GetMaxPopulation(void);

typedef enum EntityType {
  // Units
  VILLAGER,

  // Buildings
  CITY_HALL,
  SHELTER,

  // Resources
  TREE
} EntityType;

static bool TryBuild(EntityType, Vector2);

typedef struct GameTexture {
  Texture2D texture;
  int animFramesNumber;
  EntityType entityType;
} GameTexture;

static float GetGameTextureWidth(GameTexture *gameTexture) {
  return (float)gameTexture->texture.width / gameTexture->animFramesNumber;
}

typedef struct Entity {
  Vector2 position;
  Rectangle relativeHitbox;
  EntityType type;
  int hp;
  int animCurrentFrame;
  bool isSelected;
  Vector2 targetPosition;
  bool isControllable;
  int moveSpeed;
} Entity;

static Rectangle GetEntityHitbox(Entity *entity);
static bool CanMove(Vector2, Entity *);

Entity createCityHallEntity(Vector2 position) {
  return (Entity){.position = position,
                  .relativeHitbox = {103.0, 212.0, 655.0, 480.0},
                  .type = CITY_HALL,
                  .hp = 3000,
                  .animCurrentFrame = 1,
                  .isSelected = false,
                  .targetPosition = position,
                  .isControllable = false};
}

Entity createTreeEntity(Vector2 position) {
  return (Entity){.position = position,
                  .relativeHitbox = {213.0, 44.0, 80.0, 400.0},
                  .type = TREE,
                  .hp = 400,
                  .animCurrentFrame = 1,
                  .isSelected = false,
                  .targetPosition = position,
                  .isControllable = false};
}

Entity createShelterEntity(Vector2 position) {
  return (Entity){.position = position,
                  .relativeHitbox = {100.0, 230.0, 430.0, 226.0},
                  .type = SHELTER,
                  .hp = 500,
                  .animCurrentFrame = 1,
                  .isSelected = false,
                  .targetPosition = position,
                  .isControllable = false};
}

Entity createVillagerEntity(Vector2 position) {
  return (Entity){.position = position,
                  .relativeHitbox = {53.0, 9.0, 20.0, 112.0},
                  .type = VILLAGER,
                  .hp = 100,
                  .animCurrentFrame = 1,
                  .isSelected = false,
                  .targetPosition = position,
                  .isControllable = true,
                  .moveSpeed = 5};
}

struct Resources {
  int wood;
  int stone;
  int gold;
  int food;
};

enum Scene { MENU, MAIN_GAME };
enum Scene current_scene = MENU;
enum Tile { GRASS };

static GameTexture TileToTexture(enum Tile);
static GameTexture EntityToTexture(enum EntityType);

#define MAP_WIDTH 200
#define GAME_FONT_SIZE 20

static Camera2D camera = {0};
static GameTexture grassTexture;
static GameTexture primitiveCityHallTexture;
static GameTexture primitiveShelterTexture;
static GameTexture primitiveVillagerTexture;
static GameTexture treeTexture;
static Vector2 mapSize = {MAP_WIDTH, MAP_WIDTH};
static enum Tile **map = NULL;
static Entity *entities = NULL;
static int entitiesSize = 0;
static struct Resources resources;
static bool toggleHelp = false;
static GameTexture *atCursorTexture = NULL;
static bool toggleHitboxes = false;

static int GetPopulation() {
  int population = 0;
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    if (entity->isControllable)
      population++;
  }
  return population;
}

static int GetMaxPopulation() {
  int maxPopulation = BASE_POPULATION_MAX;
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    if (entity->type == SHELTER)
      maxPopulation += SHELTER_POPULATION_NUMBER;
  }
  return maxPopulation;
}

int main() {
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(0, 0, "War of progress");
  GuiLoadStyleDefault();
  InitTextures();

  RenderTexture2D target =
      LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    UpdateDrawFrame(target);
  }
#endif
  FreeGame();
  FreeTextures();
  CloseWindow();
  return 0;
}

static void UpdateDrawFrame(RenderTexture2D target) {
  BeginTextureMode(target);

  switch (current_scene) {
  case MENU:
    RenderMenu();
    break;
  case MAIN_GAME:
    CheckScroll(&camera);
    CheckMouseZoom(&camera);
    CheckSelect(&camera);
    CheckMovement(&camera);
    CheckBuilding(&camera);
    CheckInputs();
    ProcessMovements();
    RenderMainGame();
    break;
  }

  EndTextureMode();

  BeginDrawing();
  DrawTextureRec(target.texture,
                 (Rectangle){0, 0, (float)target.texture.width,
                             (float)-target.texture.height},
                 (Vector2){0, 0}, WHITE);
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

static void ProcessMovements(void) {
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    Rectangle entityHitbox = GetEntityHitbox(entity);
    int deltaMovement = GetFrameTime() * GetFPS() * entity->moveSpeed;
    if (entity->position.x < entity->targetPosition.x) {
      if (CanMove((Vector2){entityHitbox.x + entityHitbox.width + deltaMovement,
                            entityHitbox.y},
                  entity)) {
        entity->position.x += deltaMovement;
        if (entity->position.x > entity->targetPosition.x) {
          entity->position.x = entity->targetPosition.x;
        }
      }
    } else if (entity->position.x > entity->targetPosition.x) {
      if (CanMove((Vector2){entityHitbox.x - deltaMovement, entityHitbox.y},
                  entity)) {
        entity->position.x -= deltaMovement;
        if (entity->position.x < entity->targetPosition.x) {
          entity->position.x = entity->targetPosition.x;
        }
      }
    }
    if (entity->position.y < entity->targetPosition.y) {
      if (CanMove(
              (Vector2){entityHitbox.x,
                        entityHitbox.y + entityHitbox.height + deltaMovement},
              entity)) {
        entity->position.y += deltaMovement;
        if (entity->position.y > entity->targetPosition.y) {
          entity->position.y = entity->targetPosition.y;
        }
      }
    } else if (entity->position.y > entity->targetPosition.y) {
      if (CanMove((Vector2){entityHitbox.x, entityHitbox.y - deltaMovement},
                  entity)) {
        entity->position.y -= deltaMovement;
        if (entity->position.y < entity->targetPosition.y) {
          entity->position.y = entity->targetPosition.y;
        }
      }
    }
  }
}

static bool CanMove(Vector2 nextPosition, Entity *currentEntity) {
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    if (currentEntity == entity)
      continue;
    GameTexture texture = EntityToTexture(entity->type);
    Rectangle hitbox = GetEntityHitbox(entity);
    if (CheckCollisionPointRec(nextPosition, hitbox)) {
      return false;
    }
  }
  return true;
}

static Rectangle GetEntityHitbox(Entity *entity) {
  return (Rectangle){.x = entity->position.x + entity->relativeHitbox.x,
                     .y = entity->position.y + entity->relativeHitbox.y,
                     .width = entity->relativeHitbox.width,
                     .height = entity->relativeHitbox.height};
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
      GameTexture texture = TileToTexture(map[i][j]);
      DrawTexture(texture.texture, x, y, WHITE);
    }
  }

  // Draw entities
  for (i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    GameTexture texture = EntityToTexture(entity->type);
    int x = entity->position.x;
    int y = entity->position.y;
    int animWidth = texture.texture.width / texture.animFramesNumber;
    int animOffset = entity->animCurrentFrame * animWidth;
    Color textureColor = WHITE;
    if (entity->isSelected) {
      textureColor = (Color){66, 245, 102, 220};
    }
    DrawTextureRec(
        texture.texture,
        (Rectangle){animOffset, 0, animWidth, texture.texture.height},
        (Vector2){x, y}, textureColor);
    if (toggleHitboxes) {
      Rectangle entityHitbox = GetEntityHitbox(entity);
      DrawRectangleRec(entityHitbox, BLACK);
    }
    entity->animCurrentFrame++;
    if (entity->animCurrentFrame > texture.animFramesNumber) {
      entity->animCurrentFrame = 1;
    }
  }

  // May draw texture at cursor position for builds
  if (atCursorTexture) {
    Vector2 screenMousePosition = GetMousePosition();
    Vector2 mousePosition = GetScreenToWorld2D(screenMousePosition, camera);
    float textureWidth = (float)atCursorTexture->texture.width /
                         atCursorTexture->animFramesNumber;
    float textureHeight = (float)atCursorTexture->texture.height;
    Rectangle textureFrameSize = (Rectangle){0, 0, textureWidth, textureHeight};
    DrawTextureRec(atCursorTexture->texture, textureFrameSize,
                   (Vector2){mousePosition.x - textureWidth / 2,
                             mousePosition.y - textureHeight / 2},
                   (Color){255, 255, 255, 150});
  }

  EndMode2D();

  if (toggleHelp) {
    DrawHelpWindow(screenWidth, screenHeight);
  }

  DrawTopHud();
}

static void DrawHelpWindow(int screenWidth, int screenHeight) {
  DrawRectangle(screenWidth / 4, screenHeight / 4, screenWidth / 2,
                screenHeight / 2, BLACK);
  const char *helpText =
      "ACTION KEYS\nENTER - Show hitboxes\nS - Build a shelter "
      "(+5 pop). Cost:  50 wood.";
  DrawText(helpText, screenWidth / 4 + MARGIN, screenHeight / 4 + MARGIN,
           GAME_FONT_SIZE, WHITE);
}

static void DrawTopHud(void) {
  int screenWidth = GetScreenWidth();
  DrawRectangle(0, 0, screenWidth, MARGIN * 2, BLACK);
  const char *resourcesText =
      TextFormat("Wood : %i - Stone : %i - Gold : %i, Food : %i - Population : "
                 "%i/%i -  Press h for actions",
                 resources.wood, resources.stone, resources.gold,
                 resources.food, GetPopulation(), GetMaxPopulation());
  DrawText(resourcesText, MARGIN, MARGIN, GAME_FONT_SIZE, WHITE);
  DrawFPS(screenWidth - MARGIN - MeasureText("120 FPS", GAME_FONT_SIZE),
          MARGIN);
}

// ENTITIES HELPERS

static Entity CreateEntity(EntityType entityType, Vector2 position) {
  switch (entityType) {
  case VILLAGER:
    return createVillagerEntity(position);
    break;
  case CITY_HALL:
    return createCityHallEntity(position);
    break;
  case SHELTER:
    return createShelterEntity(position);
    break;
  case TREE:
    return createTreeEntity(position);
    break;
  }
}

static void AddToEntities(EntityType entityType, Vector2 position) {
  entitiesSize += 1;
  entities = realloc(entities, entitiesSize * sizeof(Entity));
  entities[entitiesSize - 1] = CreateEntity(entityType, position);
}

// SELECTED ENTITIES HELPERS

static void AddToSelectedEntities(Entity *entity) { entity->isSelected = true; }

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
  grassTexture = (GameTexture){.texture = LoadTexture("assets/map/grass.png"),
                               .animFramesNumber = 1};

  // Buildings
  primitiveCityHallTexture = (GameTexture){
      .texture = LoadTexture("assets/primitive/buildings/cityHall.png"),
      .animFramesNumber = 7,
      .entityType = CITY_HALL};
  primitiveShelterTexture = (GameTexture){
      .texture = LoadTexture("assets/primitive/buildings/shelter.png"),
      .animFramesNumber = 1,
      .entityType = SHELTER};

  // Units
  primitiveVillagerTexture = (GameTexture){
      .texture = LoadTexture("assets/primitive/units/villager.png"),
      .animFramesNumber = 1,
      .entityType = VILLAGER};

  // Resources
  treeTexture =
      (GameTexture){.texture = LoadTexture("assets/resources/tree.png"),
                    .animFramesNumber = 1,
                    .entityType = TREE};
}

static void FreeTextures(void) {
  UnloadTexture(grassTexture.texture);
  UnloadTexture(primitiveCityHallTexture.texture);
  UnloadTexture(primitiveVillagerTexture.texture);
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
  entities = (Entity *)malloc(entitiesSize * sizeof(Entity));

  int mapCenterX = camera.target.x;
  int mapCenterY = camera.target.y;

  // CITY_HALL
  entities[0] = createCityHallEntity((Vector2){mapCenterX, mapCenterY});

  // VILLAGERS
  entities[1] = createVillagerEntity((Vector2){mapCenterX - 400, mapCenterY});
  entities[2] = createVillagerEntity((Vector2){mapCenterX, mapCenterY - 400});
  entities[3] = createVillagerEntity((Vector2){mapCenterX, mapCenterY + 1100});

  const int MAP_CENTER_AREA = 1500;
  int seed = (int)time(NULL);
  perlin_init(seed);
  for (int j = 0; j < mapSize.y; j++) {
    for (int i = 0; i < mapSize.x; i++) {
      double perlinNoiseValue = perlin2D_octaves(i * 0.1f, j * 0.1f, 4, 0.5f);
      if (perlinNoiseValue > 0.25) {
        Vector2 position = {ToXIso(i, j), ToYIso(i, j)};
        if (position.x >= mapCenterX - MAP_CENTER_AREA &&
            position.x <= mapCenterX + MAP_CENTER_AREA &&
            position.y >= mapCenterY - MAP_CENTER_AREA &&
            position.y <= mapCenterY + MAP_CENTER_AREA)
          continue;
        AddToEntities(TREE, position);
      }
    }
  }
}

void InitResources(void) {
  resources.wood = 50;
  resources.stone = 50;
  resources.gold = 0;
  resources.food = 0;
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

static GameTexture TileToTexture(enum Tile tile) {
  switch (tile) {
  case GRASS:
    return grassTexture;
    break;
  }
}

// Needed and avoid pointers to textures from entities to ease resolution of
// textures depending on the current age of the player
static GameTexture EntityToTexture(enum EntityType type) { // TODO: support ages
  switch (type) {
  case CITY_HALL:
    return primitiveCityHallTexture;
    break;
  case VILLAGER:
    return primitiveVillagerTexture;
    break;
  case SHELTER:
    return primitiveShelterTexture;
    break;
  case TREE:
    return treeTexture;
    break;
  }
}

// ISOMETRIC HELPERS

static float ToXIso(int x, int y) {
  return (float)(x - y) * (grassTexture.texture.width / 2.0f);
}

static float ToYIso(int x, int y) {
  return (float)(x + y) * (grassTexture.texture.height / 4.0f);
}

static float ToXInvertedIso(int iso_x, int iso_y) {
  return (float)((iso_x / (grassTexture.texture.width / 2.0f)) +
                 (iso_y / (grassTexture.texture.height / 4.0f))) /
         2.0f;
}

static float ToYInvertedIso(int iso_x, int iso_y) {
  return (float)((iso_y / (grassTexture.texture.height / 4.0f)) -
                 (iso_x / (grassTexture.texture.width / 2.0f))) /
         2.0f;
}

// COLLISIONS HELPERS

static bool IsRightScreenHit(int x) { return x >= GetScreenWidth() - MARGIN; }

static bool IsLeftScreenHit(int x) { return x <= MARGIN; }

static bool IsBottomScreenHit(int y) { return y >= GetScreenHeight() - MARGIN; }

static bool IsTopScreenHit(int y) { return y <= MARGIN; }

// Mouse or Keyboard interactions

const int SCROLL_MOVE = 80;

static void CheckScroll(Camera2D *camera) {
  Vector2 mouse_position = GetMousePosition();
  if (IsKeyDown(KEY_RIGHT)) {
    camera->target.x += SCROLL_MOVE;
  } else if (IsKeyDown(KEY_LEFT)) {
    camera->target.x -= SCROLL_MOVE;
  }
  if (IsKeyDown(KEY_DOWN)) {
    camera->target.y += SCROLL_MOVE;
  } else if (IsKeyDown(KEY_UP)) {
    camera->target.y -= SCROLL_MOVE;
  }
  if (IsRightScreenHit(mouse_position.x)) {
    camera->target.x += SCROLL_MOVE;
  } else if (IsLeftScreenHit(mouse_position.x)) {
    camera->target.x -= SCROLL_MOVE;
  }
  if (IsBottomScreenHit(mouse_position.y)) {
    camera->target.y += SCROLL_MOVE;
  } else if (IsTopScreenHit(mouse_position.y)) {
    camera->target.y -= SCROLL_MOVE;
  }
}

static void CheckMouseZoom(Camera2D *camera) {
  float wheelDelta = GetMouseWheelMove() / 10;
  if (camera->zoom - wheelDelta > 0.1f && camera->zoom - wheelDelta < 3.0f) {
    camera->zoom -= wheelDelta;
  }
}

static void CheckSelect(Camera2D *camera) {
  if (atCursorTexture != NULL)
    return;
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    return;
  FreeSelectedEntities();
  Vector2 mousePosition = GetMousePosition();
  Vector2 mousePositionInWorld = GetScreenToWorld2D(mousePosition, *camera);
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    if (entity->isControllable == false)
      continue;
    GameTexture entityTexture = EntityToTexture(entity->type);
    int entityWidth =
        entityTexture.texture.width / (float)entityTexture.animFramesNumber;
    int entityHeight = entityTexture.texture.height;
    Vector2 entitySize = (Vector2){entityWidth, entityHeight};
    Rectangle entityHitbox = GetEntityHitbox(entity);
    if (CheckCollisionPointRec(mousePositionInWorld, entityHitbox)) {
      AddToSelectedEntities(entity);
      return;
    }
  }
}

static void CheckBuilding(Camera2D *camera) {
  if (atCursorTexture == NULL)
    return;
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    return;
  Vector2 mousePosition = GetMousePosition();
  Vector2 mousePositionInWorld = GetScreenToWorld2D(mousePosition, *camera);
  Vector2 mouseTexturePosition = {
      .x = mousePositionInWorld.x -
           (float)GetGameTextureWidth(atCursorTexture) / 2,
      .y = mousePositionInWorld.y - (float)atCursorTexture->texture.height / 2};
  Rectangle mouseTextureRectangle = {
      .x = mouseTexturePosition.x,
      .y = mouseTexturePosition.y,
      .width = (float)GetGameTextureWidth(atCursorTexture),
      .height = atCursorTexture->texture.height};
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    GameTexture entityTexture = EntityToTexture(entity->type);
    Rectangle entityHitbox = GetEntityHitbox(entity);
    if (CheckCollisionRecs(mouseTextureRectangle, entityHitbox)) {
      return;
    }
  }
  if (TryBuild(atCursorTexture->entityType, mouseTexturePosition)) {
    atCursorTexture = NULL;
  }
}

static bool TryBuild(EntityType entityType, Vector2 position) {
  switch (entityType) {
  case VILLAGER:
    // TODO
    break;
  case CITY_HALL:
    // TODO
    break;
  case SHELTER:
    if (resources.wood >= 50) {
      resources.wood -= 50;
      AddToEntities(entityType, position);
      return true;
    }
    break;
  case TREE:
    break;
  }
  return false;
}

static void CheckMovement(Camera2D *camera) {
  Vector2 mousePosition = GetMousePosition();
  if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    return;
  for (int i = 0; i < entitiesSize; i++) {
    Entity *entity = &entities[i];
    if (!entity->isControllable || !entity->isSelected) {
      continue;
    }
    Vector2 mousePositionInWorld = GetScreenToWorld2D(mousePosition, *camera);
    entity->targetPosition = mousePositionInWorld;
  }
}

static void CheckInputs() {
  if (IsKeyPressed(KEY_H)) {
    toggleHelp = !toggleHelp;
  }
  if (IsKeyPressed(KEY_S)) {
    atCursorTexture = &primitiveShelterTexture;
  }
  if (IsKeyPressed(KEY_ENTER)) {
    toggleHitboxes = !toggleHitboxes;
  }
}
