/*******************************************************************************************
 *
 *   raylib gamejam template
 *
 *   Template originally created with raylib 4.5-dev, last time updated with raylib 5.0
 *
 *   Template licensed under an unmodified zlib/libpng license, which is an OSI-certified,
 *   BSD-like license that allows static linking with closed source software
 *
 *   Copyright (c) 2022-2023 Ramon Santamaria (@raysan5)
 *
 ********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_WEB)
#define CUSTOM_MODAL_DIALOGS
#include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_PLAYER_HEALTH 100

typedef struct {
  Vector2 position;
  int health;
  float fireCooldown;
} Player;

#define BACKGROUND_PARALLAX_OFFSET 10

static const int screenWidth = 1280;
static const int screenHeight = 720;

static Shader stars = {0};
static int starsTime = 0;

static const Vector2 screen = {
  .x = screenWidth,
  .y = screenHeight,
};

static const Vector2 background = {
  .x = screenWidth + (BACKGROUND_PARALLAX_OFFSET * 2),
  .y = screenHeight + (BACKGROUND_PARALLAX_OFFSET * 2),
};

static RenderTexture2D target = {0};

static Texture2D nebulaNoise = {0};

static Texture2D interface = {0};

static Rectangle mouseCursorRect = {
  .x = 0,
  .y = 0,
  .width = 15,
  .height = 15,
};

static Vector2 mouseCursor = {0};
static Player player = {0};

static float time = 0;

typedef enum {
  PROJECTILE_NONE,
  PROJECTILE_REGULAR,
  PROJECTILE_SQUARED,
} ProjectileType;

typedef struct {
  ProjectileType type;
  float radius;
  Vector2 delta;
  Vector2 origin;

  bool isHurtfulForPlayer;
} Projectile;

#define PROJECTILES_MAX 1024

static Projectile projectiles[PROJECTILES_MAX] = {0};

#define MOUSE_SENSITIVITY 0.7f

Projectile *push_projectile(void) {
  for (int i = 0; i < PROJECTILES_MAX; i++) {
    if (projectiles[i].type == PROJECTILE_NONE) {
      return &projectiles[i];
    }
  }

  return NULL;
}

static Vector2 lookingDirection = {0};

void updateMouse(void) {
  if (IsCursorHidden()) {
    Vector2 delta =
      Vector2Multiply(GetMouseDelta(),
                      (Vector2) {
                        .x = MOUSE_SENSITIVITY,
                        .y = MOUSE_SENSITIVITY,
                      });
    mouseCursor = Vector2Add(mouseCursor, delta);
  }

  mouseCursor =
    Vector2Clamp(mouseCursor,
                 Vector2Zero(),
                 screen);

  if (!IsCursorHidden()) {
    DisableCursor();
  }

  lookingDirection = Vector2Normalize(Vector2Subtract(mouseCursor, player.position));
}

#define PLAYER_FIRE_COOLDOWN 0.1f
#define PLAYER_PROJECTILE_RADIUS 5
#define PLAYER_PROJECTILE_SPEED 10.0f

void tryFiringAShot(void) {
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
      player.fireCooldown > 0.0f) {
    return;
  }

  Projectile *new_projectile = push_projectile();

  if (new_projectile == NULL) {
    return;
  }

  *new_projectile = (Projectile) {
    .type = PROJECTILE_REGULAR,
    .isHurtfulForPlayer = false,
    .origin = Vector2Add(player.position, lookingDirection),
    .radius = PLAYER_PROJECTILE_RADIUS,
    .delta = Vector2Scale(lookingDirection, PLAYER_PROJECTILE_SPEED),
  };

  player.fireCooldown = PLAYER_FIRE_COOLDOWN;
}

#define PLAYER_MOVEMENT_SPEED 3

void updatePlayerPosition(void) {
  if (IsKeyDown(KEY_E)) {
    player.position.y -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_S)) {
    player.position.x -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_D)) {
    player.position.y += PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_F)) {
    player.position.x += PLAYER_MOVEMENT_SPEED;
  }

  player.position =
    Vector2Clamp(player.position,
                 Vector2Zero(),
                 screen);
}

void renderBackground(void) {
  float background_x = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.x / screen.x);

  float background_y = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.y / screen.y);

  SetShaderValue(stars, starsTime, &time, SHADER_UNIFORM_FLOAT);

  BeginBlendMode(BLEND_ALPHA); {
    DrawRectangle(0, 0,
                  screenWidth, screenHeight,
                  CLITERAL(Color) {
                    19, 14, 35, 255
                  });
    BeginShaderMode(stars); {
      DrawTexturePro(nebulaNoise,
                     (Rectangle) {
                       .x = background_x,
                       .y = background_y,
                       .width = nebulaNoise.width - BACKGROUND_PARALLAX_OFFSET,
                       .height = nebulaNoise.height - BACKGROUND_PARALLAX_OFFSET,
                     },
                     (Rectangle) {
                       .x = 0,
                       .y = 0,
                       .width = screen.x,
                       .height = screen.y,
                     },
                     Vector2Zero(),
                     0,
                     WHITE);
    } EndShaderMode();
  } EndBlendMode();
}

void renderPlayer(void) {
  DrawCircleV(player.position, 6, WHITE);
  DrawCircleV(player.position, 5, RED);
}

#define MOUSE_CURSOR_SCALE 2
void renderMouseCursor(void) {
  DrawTexturePro(interface,
                 mouseCursorRect,
                 (Rectangle) {
                   .x = mouseCursor.x,
                   .y = mouseCursor.y,
                   .width = mouseCursorRect.width * MOUSE_CURSOR_SCALE,
                   .height = mouseCursorRect.height * MOUSE_CURSOR_SCALE,
                 },
                 (Vector2) {
                   .x = (mouseCursorRect.width * MOUSE_CURSOR_SCALE) / 2,
                   .y = (mouseCursorRect.height * MOUSE_CURSOR_SCALE) / 2,
                 }, 0, WHITE);
}

void renderProjectiles(void) {
  for (int i = 0; i < PROJECTILES_MAX; i++) {
    switch (projectiles[i].type) {
    case PROJECTILE_NONE: break;
    case PROJECTILE_REGULAR: {
      DrawCircleV(projectiles[i].origin, projectiles[i].radius, BLUE);
      DrawCircleV(projectiles[i].origin, projectiles[i].radius - 1, WHITE);
    } break;
    case PROJECTILE_SQUARED: {
      Rectangle shape = (Rectangle) {
        .x = projectiles[i].origin.x - projectiles[i].radius,
        .y = projectiles[i].origin.y - projectiles[i].radius,
        .width = projectiles[i].radius * 2,
        .height = projectiles[i].radius * 2,
      };

      DrawRectangleRec(shape, BLUE);

      shape.x += 1;
      shape.y += 1;
      shape.width -= 2;
      shape.height -= 2;

      DrawRectangleRec(shape, WHITE);
    } break;
    }
  }
}

void renderPhase1(void) {
  BeginTextureMode(target); {
    ClearBackground(BLACK);

    renderBackground();

    renderProjectiles();

    renderPlayer();
    renderMouseCursor();
  } EndTextureMode();
}

void renderFinal(void) {
  BeginDrawing(); {
    ClearBackground(BLACK);

    float width = (float)target.texture.width;
    float height = (float)target.texture.height;

    float finalHeight = MAX((float)GetScreenHeight(), height);
    float finalWidth = finalHeight * (width / height);

    DrawTexturePro(target.texture,
                   (Rectangle) {
                     .x = 0,
                     .y = 0,
                     .width = width,
                     .height = -height
                   },
                   (Rectangle) {
                     .x = (float)GetScreenWidth() / 2,
                     .y = (float)GetScreenHeight() / 2,
                     .width = finalWidth,
                     .height = finalHeight,
                   },
                   (Vector2) { finalWidth / 2, finalHeight / 2 },
                   0.0f,
                   WHITE);

    DrawFPS(0, 0);
  } EndDrawing();
}

void updateProjectiles(void) {
  Rectangle legalArea = {
    .x = -(screen.x / 2),
    .y = -(screen.y / 2),
    .width = screen.x * 2,
    .height = screen.y * 2,
  };

  for (int i = 0; i < PROJECTILES_MAX; i++) {
    if (projectiles[i].type == PROJECTILE_NONE) {
      continue;
    }

    projectiles[i].origin = Vector2Add(projectiles[i].delta, projectiles[i].origin);

    if (!CheckCollisionPointRec(projectiles[i].origin,
                                legalArea)) {
      projectiles[i].type = PROJECTILE_NONE;
    }
  }
}

void UpdateDrawFrame(void) {
  updateProjectiles();

  updateMouse();
  updatePlayerPosition();

  player.fireCooldown -= GetFrameTime();

  tryFiringAShot();

  renderPhase1();
  renderFinal();

  time += GetFrameTime();
}

int main(void) {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "sinister");

#if defined(PLATFORM_DESKTOP)
  SetExitKey(KEY_NULL);
  SetWindowState(FLAG_WINDOW_RESIZABLE);
#endif

  DisableCursor();

  mouseCursor = (Vector2) {
    .x = screenWidth / 2,
    .y = (screenHeight / 6),
  };

  player = (Player) {
    .position = (Vector2) {
      .x = screenWidth / 2,
      .y = screenHeight - (screenHeight / 6),
    },
  };

  memset(projectiles, 0, sizeof(projectiles));

  time = 0;

  #define NEBULAE_NOISE_DOWNSCALE_FACTOR 4

  Image n = GenImagePerlinNoise(background.x / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                background.y / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                0, 0, 2);
  nebulaNoise = LoadTextureFromImage(n);
  SetTextureFilter(nebulaNoise, TEXTURE_FILTER_BILINEAR);
  UnloadImage(n);

  interface = LoadTexture("resources/ui.png");

  stars = LoadShader(NULL, TextFormat("resources/stars-%d.frag", GLSL_VERSION));
  starsTime = GetShaderLocation(stars, "time");
  SetShaderValue(stars,
                 GetShaderLocation(stars, "resolution"),
                 &background,
                 SHADER_UNIFORM_VEC2);

  target = LoadRenderTexture(screenWidth, screenHeight);

  /* fix fragTexCoord for rectangles */
  Texture2D t = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
  SetShapesTexture(t, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }
#endif

  UnloadRenderTexture(target);
  CloseWindow();

  return 0;
}
