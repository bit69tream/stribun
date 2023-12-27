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

#define MOUSE_SENSITIVITY 0.7f

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
                   screen);

    if (!IsCursorHidden()) {
      DisableCursor();
    }
  }

  #define PLAYER_MOVEMENT_SPEED 3

  {
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

  float background_x = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.x / screen.x);

  float background_y = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.y / screen.y);

  BeginTextureMode(target); {
    ClearBackground(BLACK);

    SetShaderValue(stars, starsTime, &time, SHADER_UNIFORM_FLOAT);

    BeginBlendMode(BLEND_ALPHA); {
      DrawRectangle(0, 0,
                    screenWidth, screenHeight,
                    CLITERAL(Color) {
                      /* 48, 25, 52, 255 */
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

    DrawCircleV(player.position, 6, WHITE);
    DrawCircleV(player.position, 5, RED);

#define MOUSE_CURSOR_SCALE 2
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
  } EndTextureMode();

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
  } EndDrawing();

  time += GetFrameTime();
}


int main(void) {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "sinister");

#if defined(PLATFORM_DESKTOP)
  /* SetWindowState(FLAG_WINDOW_RESIZABLE); */
#endif

  /* HideCursor(); */
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
  Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
  SetShapesTexture(texture, (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f });

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
