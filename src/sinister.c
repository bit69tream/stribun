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

#if defined(PLATFORM_WEB)
#define CUSTOM_MODAL_DIALOGS
#include <emscripten/emscripten.h>
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
} Player;

static const int screenWidth = 512;
static const int screenHeight = 512;

static RenderTexture2D target = { 0 };

static Vector2 mouseCursor = {0};
static Player player = {0};

#define MOUSE_SENSITIVITY 0.5f

void UpdateDrawFrame(void) {
  /* update mouse position */
  {
    Vector2 delta =
      Vector2Multiply(GetMouseDelta(),
                      (Vector2) {
                        .x = MOUSE_SENSITIVITY,
                        .y = MOUSE_SENSITIVITY,
                      });
    mouseCursor = Vector2Add(mouseCursor, delta);
    mouseCursor =
      Vector2Clamp(mouseCursor,
                   Vector2Zero(),
                   (Vector2) {
                     .x = screenWidth,
                     .y = screenHeight,
                   });
  }

  #define PLAYER_MOVEMENT_SPEED 3

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

  BeginTextureMode(target); {
    ClearBackground(BLACK);

    DrawRectangle(0, 0, screenWidth, screenHeight, SKYBLUE);

    DrawCircleV(player.position, 6, WHITE);
    DrawCircleV(player.position, 5, RED);

    DrawCircleV(mouseCursor, 5, RED);
  } EndTextureMode();

  BeginDrawing(); {
    ClearBackground(BLACK);

    float width = (float)target.texture.width;
    float height = (float)target.texture.height;

    float aspect = width / height;

    float finalHeight = MAX((float)GetScreenHeight(), height);
    float finalWidth = finalHeight;

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
  }
  EndDrawing();
}


int main(void) {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "sinister");

#if defined(PLATFORM_DESKTOP)
  SetWindowState(FLAG_WINDOW_RESIZABLE);
#endif

  HideCursor();
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

  target = LoadRenderTexture(screenWidth, screenHeight);
  /* SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR); */

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
