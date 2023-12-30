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
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef enum {
  DIRECTION_UP    = 0b0001,
  DIRECTION_DOWN  = 0b0010,
  DIRECTION_LEFT  = 0b0100,
  DIRECTION_RIGHT = 0b1000,
} Direction;

#define MAX_PLAYER_HEALTH 100

#define PLAYER_HITBOX_RADIUS 16

#define PLAYER_DASH_COOLDOWN 0.5f

#define PLAYER_MOVEMENT_SPEED 6

typedef struct {
  Vector2 position;

  int health;

  float fireCooldown;
  float dashCooldown;

  Direction movementDirection;
  Vector2 movementDelta;

  float dashReactivationEffectAlpha;
  Vector2 dashDelta;

  bool isInvincible;
} Player;

#define BACKGROUND_PARALLAX_OFFSET 10

static const int screenWidth = 1280;
static const int screenHeight = 720;

static Shader arenaBorderShader = {0};
static int arenaBorderTime = 0;

static Shader stars = {0};
static int starsTime = 0;

static Camera2D camera = {0};

#define LEVEL_WIDTH 2048
#define LEVEL_HEIGHT 2048

static const Vector2 level = {
  .x = LEVEL_WIDTH,
  .y = LEVEL_HEIGHT,
};

static const Vector2 background = {
  .x = LEVEL_WIDTH + (BACKGROUND_PARALLAX_OFFSET * 2),
  .y = LEVEL_HEIGHT + (BACKGROUND_PARALLAX_OFFSET * 2),
};

static RenderTexture2D target = {0};

static Texture2D nebulaNoise = {0};

static Texture2D sprites = {0};

static Rectangle mouseCursorRect = {
  .x = 0,
  .y = 0,
  .width = 15,
  .height = 15,
};

static Rectangle playerRect = {
  .x = 0,
  .y = 16,
  .width = 17,
  .height = 19,
};

static Vector2 mouseCursor = {0};
static Player player = {0};

static float time = 0;

static Sound dashSoundEffect = {0};
static Sound playerShot = {0};

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

  bool willBeDestroyed;
  float destructionTimer;
  bool isHurtfulForPlayer;
  /* TODO: make some projectiles able to bounce from the arena border */
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

static Vector2 screenMouseLocation = {0};

void updateMouse(void) {
  if (IsCursorHidden()) {
    Vector2 delta =
      Vector2Multiply(GetMouseDelta(),
                      (Vector2) {
                        .x = MOUSE_SENSITIVITY,
                        .y = MOUSE_SENSITIVITY,
                      });
    screenMouseLocation = Vector2Add(screenMouseLocation, delta);

    screenMouseLocation.x = Clamp(screenMouseLocation.x,
                                  0.0f,
                                  GetScreenWidth());

    screenMouseLocation.y = Clamp(screenMouseLocation.y,
                                  0.0f,
                                  GetScreenHeight());
  }

  mouseCursor = GetScreenToWorld2D(screenMouseLocation, camera);

  if (!IsCursorHidden()) {
    DisableCursor();
  }

  lookingDirection = Vector2Normalize(Vector2Subtract(mouseCursor, player.position));
}

#define PLAYER_DASH_DISTANCE 128

void tryDashing(void) {
  if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
      player.dashCooldown > 0.0f) {
    return;
  }

  Vector2 direction = Vector2Normalize(player.movementDelta);

  if (direction.x == 0.0f && direction.y == 0.0f) {
    return;
  }

  static const Vector2 up = {
    .x = 0,
    .y = -1,
  };

  float dashAngle = Vector2Angle(up, direction);

  Vector2 dashDistance = Vector2Scale(up, PLAYER_DASH_DISTANCE);

  player.dashCooldown = PLAYER_DASH_COOLDOWN;
  player.dashDelta = Vector2Rotate(dashDistance, dashAngle);
  player.isInvincible = true;

  PlaySound(dashSoundEffect);
}

#define PLAYER_FIRE_COOLDOWN 0.15f
#define PLAYER_PROJECTILE_RADIUS 9
#define PLAYER_PROJECTILE_SPEED 7.0f

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
    .origin = Vector2Add(player.position, Vector2Scale(lookingDirection, 25)),
    .radius = PLAYER_PROJECTILE_RADIUS,
    .delta = Vector2Scale(lookingDirection, PLAYER_PROJECTILE_SPEED),
  };

  player.fireCooldown = PLAYER_FIRE_COOLDOWN;

  PlaySound(playerShot);
}

void updatePlayerPosition(void) {
  player.movementDirection = 0;
  player.movementDelta = Vector2Zero();

  /* TODO: settings option to change movement keys */

  if (IsKeyDown(KEY_E)) {
    player.movementDirection |= DIRECTION_UP;
    player.movementDelta.y -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_S)) {
    player.movementDirection |= DIRECTION_LEFT;
    player.movementDelta.x -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_D)) {
    player.movementDirection |= DIRECTION_DOWN;
    player.movementDelta.y += PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(KEY_F)) {
    player.movementDirection |= DIRECTION_RIGHT;
    player.movementDelta.x += PLAYER_MOVEMENT_SPEED;
  }

  player.position =
    Vector2Add(player.position,
               player.movementDelta);

#define DASH_DELTA_LERP_RATE 0.5f
  player.dashDelta.x = Lerp(player.dashDelta.x,
                            0.0f,
                            DASH_DELTA_LERP_RATE);

  player.dashDelta.y = Lerp(player.dashDelta.y,
                            0.0f,
                            DASH_DELTA_LERP_RATE);

  player.isInvincible = (roundf(player.dashDelta.x) != 0.0f ||
                         roundf(player.dashDelta.y) != 0.0f);

  player.position =
    Vector2Add(player.position,
               player.dashDelta);

  player.position =
    Vector2Clamp(player.position,
                 Vector2Zero(),
                 level);
}

void renderBackground(void) {
  float background_x = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.x / LEVEL_WIDTH);

  float background_y = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            player.position.y / LEVEL_HEIGHT);

  SetShaderValue(stars, starsTime, &time, SHADER_UNIFORM_FLOAT);

  BeginBlendMode(BLEND_ALPHA); {
    ClearBackground(BLACK);
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
                       .width = LEVEL_WIDTH,
                       .height = LEVEL_HEIGHT,
                     },
                     Vector2Zero(),
                     0,
                     WHITE);
    } EndShaderMode();
  } EndBlendMode();
}

static RenderTexture2D playerTexture = {0};

#define THRUSTERS_BOTTOM 0b0001
#define THRUSTERS_TOP    0b0010
#define THRUSTERS_RIGHT  0b0100
#define THRUSTERS_LEFT   0b1000

static Rectangle thrustersRects[] = {
  [THRUSTERS_BOTTOM] = {
    .x = 17,
    .y = 30,
    .width = 17,
    .height = 4,
  },

  [THRUSTERS_TOP] = {
    .x = 17,
    .y = 16,
    .width = 17,
    .height = 4,
  },

  [THRUSTERS_LEFT] = {
    .x = 17,
    .y = 16,
    .width = 3,
    .height = 18,
  },

  [THRUSTERS_RIGHT] = {
    .x = 31,
    .y = 16,
    .width = 3,
    .height = 18,
  },
};

float playerLookingAngle(void) {
  static const Vector2 up = {
    .x = 0,
    .y = -1,
  };

  float angle = Vector2Angle(up, lookingDirection) * RAD2DEG;

  return angle;
}

int whichThrustersToUse(void) {
  float angle = playerLookingAngle();

  /* angle from playerLookingAngle() function is inside of the (-180; 180) range */

  if (angle < 0.0f) {
    angle = 360.0f + angle;
  }

  angle = Clamp(angle, 0.0f, 360.0f);

  angle += 45.0f;

  if (angle >= 360.0f) {
    angle = angle - 360.0f;
  }

  Direction facing = 0;

  if (angle >= 0.0f && angle <= 90.0f) {
    facing = DIRECTION_UP;
  } else if (angle >= 90.0f && angle <= 180.0f) {
    facing = DIRECTION_RIGHT;
  } else if (angle >= 180.0f && angle <= 270.0f) {
    facing = DIRECTION_DOWN;
  } else {
    facing = DIRECTION_LEFT;
  }

  int thrusters = 0;

#define THRUST_RULES                            \
  do {                                          \
    X(UP, UP, BOTTOM);                          \
    X(UP, LEFT, RIGHT);                         \
    X(UP, DOWN, TOP);                           \
    X(UP, RIGHT, LEFT);                         \
                                                \
    X(LEFT, UP, LEFT);                          \
    X(LEFT, LEFT, BOTTOM);                      \
    X(LEFT, DOWN, RIGHT);                       \
    X(LEFT, RIGHT, TOP);                        \
                                                \
    X(DOWN, UP, TOP);                           \
    X(DOWN, LEFT, LEFT);                        \
    X(DOWN, DOWN, BOTTOM);                      \
    X(DOWN, RIGHT, RIGHT);                      \
                                                \
    X(RIGHT, UP, RIGHT);                        \
    X(RIGHT, LEFT, TOP);                        \
    X(RIGHT, DOWN, LEFT);                       \
    X(RIGHT, RIGHT, BOTTOM);                    \
  } while (0)

#define X(f, m, t) if (facing == DIRECTION_##f && player.movementDirection & DIRECTION_##m) thrusters |= THRUSTERS_##t

  THRUST_RULES;

#undef X
#undef THRUST_RULES

  return thrusters;
}

void renderThrusters(int thrusters) {
  Color a = Fade(WHITE, 0.85f);

  if (thrusters & THRUSTERS_BOTTOM) {
    DrawTexturePro(sprites,
                   thrustersRects[THRUSTERS_BOTTOM],
                   (Rectangle) {
                     .x = 0,
                     .y = 14,
                     .width = thrustersRects[THRUSTERS_BOTTOM].width,
                     .height = thrustersRects[THRUSTERS_BOTTOM].height,
                   },
                   Vector2Zero(),
                   0,
                   a);
  }

  if (thrusters & THRUSTERS_TOP) {
    DrawTexturePro(sprites,
                   thrustersRects[THRUSTERS_TOP],
                   (Rectangle) {
                     .x = 0,
                     .y = 0,
                     .width = thrustersRects[THRUSTERS_TOP].width,
                     .height = thrustersRects[THRUSTERS_TOP].height,
                   },
                   Vector2Zero(),
                   0,
                   a);
  }


  if (thrusters & THRUSTERS_LEFT) {
    DrawTexturePro(sprites,
                   thrustersRects[THRUSTERS_LEFT],
                   (Rectangle) {
                     .x = 0,
                     .y = 0,
                     .width = thrustersRects[THRUSTERS_LEFT].width,
                     .height = thrustersRects[THRUSTERS_LEFT].height,
                   },
                   Vector2Zero(),
                   0,
                   a);
  }

  if (thrusters & THRUSTERS_RIGHT) {
    DrawTexturePro(sprites,
                   thrustersRects[THRUSTERS_RIGHT],
                   (Rectangle) {
                     .x = 14,
                     .y = 0,
                     .width = thrustersRects[THRUSTERS_RIGHT].width,
                     .height = thrustersRects[THRUSTERS_RIGHT].height,
                   },
                   Vector2Zero(),
                   0,
                   a);
  }
}

typedef struct {
  RenderTexture2D texture;
  float alpha;
  Vector2 origin;
  float angle;
} ThrusterTrail;

#define THRUSTER_TRAILS_MAX 10
static ThrusterTrail thrusterTrail[THRUSTER_TRAILS_MAX] = {0};

ThrusterTrail *pushThrusterTrail() {
  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    if (thrusterTrail[i].alpha <= 0.0f) {
      return &thrusterTrail[i];
    }
  }

  return NULL;
}

static Shader dashResetShader = {0};
static int dashResetShaderAlpha = 0;

void renderPlayerTexture(void) {
  int thrusters = whichThrustersToUse();

  BeginTextureMode(playerTexture); {
    ClearBackground(BLANK);

    SetShaderValue(dashResetShader,
                   dashResetShaderAlpha,
                   &player.dashReactivationEffectAlpha,
                   SHADER_UNIFORM_FLOAT);

    BeginShaderMode(dashResetShader); {
      DrawTexturePro(sprites,
                     playerRect,
                     (Rectangle) {
                       .x = 0,
                       .y = 0,
                       .width = playerRect.width,
                       .height = playerRect.height,
                     },
                     Vector2Zero(),
                     0,
                     WHITE);
    } EndShaderMode();

    renderThrusters(thrusters);
  } EndTextureMode();

  if (thrusters == 0) {
    return;
  }

  ThrusterTrail *t = pushThrusterTrail();

  if (t == NULL) {
    return;
  }

  BeginTextureMode(t->texture); {
    ClearBackground(BLANK);

    renderThrusters(thrusters);
  } EndTextureMode();

  t->angle = playerLookingAngle();
  t->alpha = 1.0f;
  t->origin = player.position;
}

#define PLAYER_SCALE 3.5
void renderPlayer(void) {
  DrawTexturePro(playerTexture.texture,
                 (Rectangle) {
                   .x = 0,
                   .y = 0,
                   .width = playerTexture.texture.width,
                   .height = -playerTexture.texture.height,
                 },
                 (Rectangle) {
                   .x = player.position.x,
                   .y = player.position.y,
                   .width = playerTexture.texture.width * PLAYER_SCALE,
                   .height = playerTexture.texture.height * PLAYER_SCALE,
                 },
                 (Vector2) {
                   .x = (playerRect.width * PLAYER_SCALE) / 2,
                   .y = (playerRect.height * PLAYER_SCALE) / 2,
                 },
                 playerLookingAngle(),
                 WHITE);

  /* if (player.isInvincible) */
  /*   DrawCircleV(player.position, PLAYER_HITBOX_RADIUS, RED); */
}

void renderThrusterTrails(void) {
  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    if (thrusterTrail[i].alpha <= 0.0f) {
      continue;
    }

    DrawTexturePro(thrusterTrail[i].texture.texture,
                   (Rectangle) {
                     .x = 0,
                     .y = 0,
                     .width = thrusterTrail[i].texture.texture.width,
                     .height = -thrusterTrail[i].texture.texture.height,
                   },
                   (Rectangle) {
                     .x = thrusterTrail[i].origin.x,
                     .y = thrusterTrail[i].origin.y,
                     .width = thrusterTrail[i].texture.texture.width * PLAYER_SCALE,
                     .height = thrusterTrail[i].texture.texture.height * PLAYER_SCALE,
                   },
                   (Vector2) {
                     .x = (playerRect.width * PLAYER_SCALE) / 2,
                     .y = (playerRect.height * PLAYER_SCALE) / 2,
                   },
                   thrusterTrail[i].angle,
                   Fade(WHITE, thrusterTrail[i].alpha));
  }
}

void updateThrusterTrails(void) {
  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    if (thrusterTrail[i].alpha <= 0.0f) {
      continue;
    }

    thrusterTrail[i].alpha = Clamp(thrusterTrail[i].alpha - 0.2f, 0.0f, 1.0f);
  }
}

#define MOUSE_CURSOR_SCALE 2
void renderMouseCursor(void) {
  DrawTexturePro(sprites,
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

#define PROJECTILE_BORDER 3

void renderProjectiles(void) {
  for (int i = 0; i < PROJECTILES_MAX; i++) {
    switch (projectiles[i].type) {
    case PROJECTILE_NONE: break;
    case PROJECTILE_REGULAR: {
      DrawCircleV(projectiles[i].origin,
                  projectiles[i].radius,
                  BLUE);

      if (projectiles[i].willBeDestroyed) {
        break;
      }

      DrawCircleV(projectiles[i].origin,
                  projectiles[i].radius - PROJECTILE_BORDER,
                  WHITE);
    } break;
    case PROJECTILE_SQUARED: {
      Rectangle shape = (Rectangle) {
        .x = projectiles[i].origin.x - projectiles[i].radius,
        .y = projectiles[i].origin.y - projectiles[i].radius,
        .width = projectiles[i].radius * 2,
        .height = projectiles[i].radius * 2,
      };

      DrawRectangleRec(shape, BLUE);

      if (projectiles[i].willBeDestroyed) {
        break;
      }

      shape.x += PROJECTILE_BORDER;
      shape.y += PROJECTILE_BORDER;
      shape.width -= PROJECTILE_BORDER * 2;
      shape.height -= PROJECTILE_BORDER * 2;

      DrawRectangleRec(shape, WHITE);
    } break;
    }
  }
}

void renderArenaBorder(void) {
  SetShaderValue(arenaBorderShader,
                 arenaBorderTime,
                 &time,
                 SHADER_UNIFORM_FLOAT);
  BeginShaderMode(arenaBorderShader); {
    DrawRectangle(0, 0,
                  LEVEL_WIDTH, LEVEL_HEIGHT,
                  BLUE);
  } EndShaderMode();
}

void renderPhase1(void) {
  renderPlayerTexture();

  BeginTextureMode(target); {
    ClearBackground(BLACK);

    renderBackground();
    renderArenaBorder();
    renderThrusterTrails();
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

    BeginMode2D(camera); {
      DrawTexturePro(target.texture,
                     (Rectangle) {
                       .x = 0,
                       .y = 0,
                       .width = width,
                       .height = -height
                     },
                     (Rectangle) {
                       .x = 0,
                       .y = 0,
                       .width = width,
                       .height = height
                     },
                     Vector2Zero(),
                     0.0f,
                     WHITE);
    }; EndMode2D();

    DrawFPS(0, 0);
  } EndDrawing();
}

void updateProjectiles(void) {
  for (int i = 0; i < PROJECTILES_MAX; i++) {
    if (projectiles[i].type == PROJECTILE_NONE) {
      continue;
    }

    projectiles[i].destructionTimer = Clamp(projectiles[i].destructionTimer - GetFrameTime(),
                                            0,
                                            1.0f);

    if (projectiles[i].willBeDestroyed) {
      if (projectiles[i].destructionTimer <= 0) {
        projectiles[i].type = PROJECTILE_NONE;
      }

      continue;
    }

    projectiles[i].origin = Vector2Add(projectiles[i].delta, projectiles[i].origin);

    if ((projectiles[i].origin.x <= 0) ||
        (projectiles[i].origin.y <= 0) ||
        (projectiles[i].origin.x >= (LEVEL_WIDTH - 1)) ||
        (projectiles[i].origin.y >= (LEVEL_HEIGHT - 1))) {
      projectiles[i].origin.x = Clamp(projectiles[i].origin.x,
                                      0,
                                      LEVEL_WIDTH - 1);
      projectiles[i].origin.y = Clamp(projectiles[i].origin.y,
                                      0,
                                      LEVEL_HEIGHT - 1);

      projectiles[i].willBeDestroyed = true;
      projectiles[i].destructionTimer = 0.05f;
    }
  }
}

void updatePlayerCooldowns(void) {
  float frameTime = GetFrameTime();

  bool dashCooldownActive = player.dashCooldown > 0.0f;

#define COOLDOWN_MAX 10.0f
#define DECREASE_COOLDOWN(c) (c) = Clamp((c) - frameTime, 0.0f, COOLDOWN_MAX)

  DECREASE_COOLDOWN(player.fireCooldown);
  DECREASE_COOLDOWN(player.dashCooldown);

  frameTime *= 2;
  DECREASE_COOLDOWN(player.dashReactivationEffectAlpha);

#undef DECREASE_COOLDOWN

  if (player.dashCooldown <= 0.0f && dashCooldownActive) {
    player.dashReactivationEffectAlpha = 0.5f;
  }
}

void updateCamera(void) {
  float windowWidth = GetScreenWidth();
  float windowHeight = GetScreenHeight();

  float x = windowWidth / (float)screenWidth;
  float y = windowHeight / (float)screenHeight;

  camera.zoom = MIN(x, y);

  float halfScreenWidth = (windowWidth / (2 * camera.zoom));
  float halfScreenHeight = (windowHeight / (2 * camera.zoom));

  float target_x = Clamp(player.position.x, halfScreenWidth,
                         (float)LEVEL_WIDTH - halfScreenWidth);
  float target_y = Clamp(player.position.y, halfScreenHeight,
                         (float)LEVEL_HEIGHT - halfScreenHeight);

  camera.target.x = Lerp(camera.target.x, target_x, 0.1f);
  camera.target.y = Lerp(camera.target.y, target_y, 0.1f);

  camera.offset.x = windowWidth / 2.0f;
  camera.offset.y = windowHeight / 2.0f;
}

void UpdateDrawFrame(void) {
  updateCamera();

  updateProjectiles();
  updateThrusterTrails();

  updateMouse();
  updatePlayerPosition();
  updatePlayerCooldowns();

  tryDashing();
  tryFiringAShot();

  renderPhase1();
  renderFinal();

  time += GetFrameTime();
}

int main(void) {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "stribun");
  InitAudioDevice();

#if defined(PLATFORM_DESKTOP)
  SetExitKey(KEY_NULL);
  SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED | FLAG_BORDERLESS_WINDOWED_MODE);
#endif

  DisableCursor();

  screenMouseLocation = (Vector2) {
    .x = (float)screenWidth / 2,
    .y = ((float)screenHeight / 6),
  };

  memset(&player, 0, sizeof(player));

  camera.rotation = 0;
  camera.zoom = 1;

  player = (Player) {
    .position = (Vector2) {
      .x = (float)LEVEL_WIDTH / 2,
      .y = LEVEL_HEIGHT - ((float)LEVEL_HEIGHT / 6),
    },
    .isInvincible = false,
  };

  memset(projectiles, 0, sizeof(projectiles));

  time = 0;

  dashSoundEffect = LoadSound("resources/dash.wav");
  SetSoundVolume(dashSoundEffect, 0.3);

  playerShot = LoadSound("resources/shot01.wav");
  SetSoundVolume(playerShot, 0.2);

  #define NEBULAE_NOISE_DOWNSCALE_FACTOR 8

  Image n = GenImagePerlinNoise(background.x / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                background.y / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                0, 0, 4);
  nebulaNoise = LoadTextureFromImage(n);
  SetTextureFilter(nebulaNoise, TEXTURE_FILTER_BILINEAR);
  UnloadImage(n);

  sprites = LoadTexture("resources/sprites.png");

  Vector4 arenaBorderColor0 = ColorNormalize(DARKBLUE);
  Vector4 arenaBorderColor1 = ColorNormalize(BLUE);

  arenaBorderShader = LoadShader(NULL, TextFormat("resources/border-%d.frag", GLSL_VERSION));
  arenaBorderTime = GetShaderLocation(arenaBorderShader, "time");
  SetShaderValue(arenaBorderShader,
                 GetShaderLocation(arenaBorderShader, "borderColor0"),
                 &arenaBorderColor0,
                 SHADER_ATTRIB_VEC4);
  SetShaderValue(arenaBorderShader,
                 GetShaderLocation(arenaBorderShader, "borderColor1"),
                 &arenaBorderColor1,
                 SHADER_ATTRIB_VEC4);

  Vector4 dashResetGlowColor = ColorNormalize(ColorAlpha(SKYBLUE, 0.1f));

  dashResetShader = LoadShader(NULL, TextFormat("resources/dash-reset-glow-%d.frag", GLSL_VERSION));
  dashResetShaderAlpha = GetShaderLocation(dashResetShader, "alpha");
  SetShaderValue(dashResetShader,
                 GetShaderLocation(dashResetShader, "glowColor"),
                 &dashResetGlowColor,
                 SHADER_UNIFORM_VEC4);

  stars = LoadShader(NULL, TextFormat("resources/stars-%d.frag", GLSL_VERSION));
  starsTime = GetShaderLocation(stars, "time");
  SetShaderValue(stars,
                 GetShaderLocation(stars, "resolution"),
                 &background,
                 SHADER_UNIFORM_VEC2);

  target = LoadRenderTexture(LEVEL_WIDTH, LEVEL_HEIGHT);

  playerTexture = LoadRenderTexture(playerRect.width, playerRect.height);

  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    thrusterTrail[i].texture = LoadRenderTexture(playerRect.width, playerRect.height);
    thrusterTrail[i].alpha = 0.0f;
    thrusterTrail[i].origin = Vector2Zero();
    thrusterTrail[i].angle = 0.0f;
  }

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
