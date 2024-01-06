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
#include <math.h>

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

static bool seenTutorial = false;

static bool esdf = false;

typedef enum {
  BOSS_INTRODUCTION_BEGINNING,
  BOSS_INTRODUCTION_FOCUS,
  BOSS_INTRODUCTION_INFO,
} BossIntroductionStage;

static BossIntroductionStage introductionStage = {0};

typedef enum {
  GAME_MAIN_MENU,
  GAME_TUTORIAL,
  GAME_BOSS_INTRODUCTION,
  GAME_BOSS,
  GAME_BOSS_DEAD,
  GAME_PLAYER_DEAD,
  GAME_STATS,
} GameState;

static bool isGamePaused = false;

static Vector2 cameraIntroductionTarget = {0};

static Music mainMenuMusic = {0};
static Music bossMarineMusic = {0};

static GameState gameState = GAME_MAIN_MENU;

#define SPRITES_SCALE 3.5
static Texture2D sprites = {0};

typedef enum {
  DIRECTION_UP    = 0b0001,
  DIRECTION_DOWN  = 0b0010,
  DIRECTION_LEFT  = 0b0100,
  DIRECTION_RIGHT = 0b1000,
} Direction;

#define MAX_PLAYER_HEALTH 8

#define PLAYER_HITBOX_RADIUS 16

#define PLAYER_DASH_COOLDOWN 0.55f

#define PLAYER_MOVEMENT_SPEED 6

typedef struct {
  Vector2 position;

  int health;

  float fireCooldown;
  float dashCooldown;

  Direction movementDirection;
  Vector2 movementDelta;
  Vector2 dashDelta;

  float dashReactivationEffectAlpha;

  int bulletSpread;

  bool isInvincible;
  float iframeTimer;
} Player;

typedef struct {
  float time;
  int kills;
} PlayerStats;

static PlayerStats playerStats = {0};

#define MAX_BOUNDING_CIRCLES 3

typedef struct {
  Vector2 position;
  float radius;
} Circle;

static Rectangle bossMarineRect = {
  .x = 0,
  .y = 35,
  .width = 71,
  .height = 71,
};

static Rectangle bossMarineWeaponRect = {
  .x = 76,
  .y = 61,
  .width = 85,
  .height = 37,
};

static Vector2 bossMarineInitialWeaponOffset = {
  .x = 14 * SPRITES_SCALE,
  .y = 6 * SPRITES_SCALE,
};

#define BOSS_MARINE_MAX_HEALTH 512

#define BOSS_MARINE_NAME "Uziel Enkidas"

#define BOSS_MARINE_BOUNDING_CIRCLES 12

typedef enum {
  BOSS_MARINE_SINUS_SHOOTING,
  BOSS_MARINE_SHOOTING,
  BOSS_MARINE_SHOTGUNNING,
  BOSS_MARINE_BOUNCING_WAVES,
  BOSS_MARINE_NOT_SHOOTING,
} BossMarineAttack;

typedef struct {
  Vector2 position;
  int health;

  /* positions are relative */
  Circle boundingCircles[BOSS_MARINE_BOUNDING_CIRCLES];

  float horizontalFlip;
  Circle processedBoundingCircles[BOSS_MARINE_BOUNDING_CIRCLES];
  Vector2 bulletOrigin;
  float weaponAngle;
  Vector2 weaponOffset;

  float walkingDirection;
  bool isWalking;
  BossMarineAttack currentAttack;
  float attackTimer;
  float weaponAngleOffset;
  float fireCooldown;
} BossMarine;

BossMarine bossMarine = {0};

typedef enum {
  BOSS_MARINE,
  BOSS_BALL,
} BossType;

BossType currentBoss = BOSS_MARINE;

#define BACKGROUND_PARALLAX_OFFSET 16

static const int screenWidth = 1280;
static const int screenHeight = 720;

static Shader arenaBorderShader = {0};
static int arenaBorderTime = 0;

static Shader stars = {0};
static int starsTime = 0;

static Shader dashTrailShader = {0};
static int dashTrailShaderAlpha = {0};

static Camera2D camera = {0};

#define LEVEL_WIDTH 1536
#define LEVEL_HEIGHT 1536

static const Vector2 level = {
  .x = LEVEL_WIDTH,
  .y = LEVEL_HEIGHT,
};

static const Vector2 background = {
  .x = (float)LEVEL_WIDTH * 1.5,
  .y = (float)LEVEL_HEIGHT * 1.5,
};

static RenderTexture2D target = {0};

static Texture2D nebulaNoise = {0};

#define BIG_ASS_ASTEROID_SCALE 15

static Rectangle bigAssAsteroidRect = {
  .x = 121,
  .y = 0,
  .width = 35,
  .height = 29,
};
static Vector2 bigAssAsteroidPosition = {0};
static float bigAssAsteroidAngle = 0;
static Vector2 bigAssAsteroidPositionDelta = {0};
static float bigAssAsteroidAngleDelta = 0;

static Rectangle mouseCursorRect = {
  .x = 175,
  .y = 0,
  .width = 17,
  .height = 17,
};

static Rectangle playerHealthOverlayMaskRect = {
  .x = 16,
  .y = 0,
  .width = 15,
  .height = 16,
};

static Shader playerHealthOverlayShader = {0};
static int playerHealthOverlayHealth = 0;
static Texture2D playerHealthMaskTexture = {0};

static Rectangle playerRect = {
  .x = 0,
  .y = 16,
  .width = 17,
  .height = 19,
};

typedef struct {
  Rectangle textureRect;
  int boundingCirclesLen;
  /* NOTE: positions are relative to the center of the texture */
  Circle boundingCircles[MAX_BOUNDING_CIRCLES];
} AsteroidSprite;

static const AsteroidSprite asteroidSprites[] = {
  {
    .textureRect = {
      .x = 34,
      .y = 0,
      .width = 19,
      .height = 19,
    },
    .boundingCirclesLen = 1,
    .boundingCircles = {
      {
        .position = {0, 0},
        .radius = 9,
      }
    },
  },
  {
    .textureRect = {
      .x = 55,
      .y = 0,
      .width = 35,
      .height = 25,
    },
    .boundingCirclesLen = 3,
    .boundingCircles = {
      {
        .position = {6, -2},
        .radius = 11,
      },
      {
        .position = {4, 7},
        .radius = 5,
      },
      {
        .position = {-6, 1},
        .radius = 12,
      }
    }
  },
  {
    .textureRect = {
      .x = 91,
      .y = 0,
      .width = 27,
      .height = 25,
    },
    .boundingCirclesLen = 1,
    .boundingCircles = {
      {
        .position = {0, 0},
        .radius = 13,
      }
    }
  }
};

typedef struct {
  const AsteroidSprite * sprite;
  float angle;
  float angleDelta;
  Vector2 position;
  Vector2 delta;

  Circle processedBoundingCircles[MAX_BOUNDING_CIRCLES];
  bool launchedByPlayer;
} Asteroid;

#define MIN_ASTEROIDS 4
#define MAX_ASTEROIDS 10
static Asteroid asteroids[MAX_ASTEROIDS] = {0};
static int asteroidsLen = 0;

static Vector2 mouseCursor = {0};
static Player player = {0};

static float time = 0;

static Sound playerDeathSound = {0};

static Sound dashSoundEffect = {0};
static Sound playerShot = {0};
static Sound beep = {0};
static Sound hit = {0};
static Sound buttonFocusEffect = {0};
static Sound borderActivation = {0};

static Sound bossMarineShotgunSound = {0};
static Sound bossMarineGunshotSound = {0};

typedef enum {
  PROJECTILE_NONE,
  PROJECTILE_REGULAR,
  PROJECTILE_SQUARED,
} ProjectileType;

typedef struct {
  ProjectileType type;

  union {
    float radius;
    Vector2 size;
  };

  Vector2 delta;
  Vector2 origin;
  float angle;

  bool willBeDestroyed;
  float destructionTimer;

  bool isHurtfulForPlayer;
  bool isHurtfulForBoss;

  int damage;

  Color inside;
  Color outside;

  float lifetime;
  bool canBounce;
} Projectile;

#define PROJECTILES_MAX 1024

static Projectile projectiles[PROJECTILES_MAX] = {0};

#define MOUSE_SENSITIVITY 0.7f

float mod(float v, float max) {
  if (v > max) {
    return fmodf(v, max);
  }

  if (v < 0) {
    return max - fmodf(v, max);
  }

  return v;
}

float angleBetweenPoints(Vector2 p1, Vector2 p2) {
  Vector2 d = Vector2Subtract(p1, p2);
  float dist = sqrt((d.x * d.x) + (d.y * d.y));

  float alpha = asin(d.x / dist) * RAD2DEG;

  if (d.y > 0) {
    alpha = copysignf(180 - fabsf(alpha), alpha);
  }

  alpha += 180;

  return alpha;
}

void checkForCollisionsBetweenAsteroids(int i, int k) {
  for (int bi = 0; bi < asteroids[i].sprite->boundingCirclesLen; bi++) {
    Vector2 ipos = Vector2Add(asteroids[i].position,
                              asteroids[i].processedBoundingCircles[bi].position);

    for (int bk = 0; bk < asteroids[k].sprite->boundingCirclesLen; bk++) {
      Vector2 kpos = Vector2Add(asteroids[k].position,
                                asteroids[k].processedBoundingCircles[bk].position);

      float distance = Vector2Distance(kpos, ipos);
      float radiusSum =
        (asteroids[k].processedBoundingCircles[bk].radius +
         asteroids[i].processedBoundingCircles[bi].radius);

      if (distance < radiusSum) {
        float angle = angleBetweenPoints(kpos, ipos) *
          DEG2RAD;

        float offsetDistance = distance - radiusSum;
        Vector2 offset = Vector2Rotate((Vector2) {0, offsetDistance}, angle);
        asteroids[i].position = Vector2Add(asteroids[i].position, offset);

        asteroids[i].delta = Vector2Add(asteroids[i].delta, offset);
        asteroids[k].delta = Vector2Subtract(asteroids[k].delta, offset);

        asteroids[i].launchedByPlayer = false;
        asteroids[k].launchedByPlayer = true;
        return;
      }
    }
  }
}

void checkForCollisionsBetweenAsteroidsAndBorders(void) {
  Vector2 normalDown = {0, 1};
  Vector2 normalUp = {0, -1};
  Vector2 normalRight = {1, 0};
  Vector2 normalLeft = {-1, 0};

  for (int i = 0; i < asteroidsLen; i++) {
    for (int j = 0; j < asteroids[i].sprite->boundingCirclesLen; j++) {
      Vector2 pos = Vector2Add(asteroids[i].position,
                               asteroids[i].processedBoundingCircles[j].position);

      float r = asteroids[i].processedBoundingCircles[j].radius;

      if ((pos.y - r) <= 0) {
        asteroids[i].delta = Vector2Reflect(asteroids[i].delta, normalDown);
        asteroids[i].launchedByPlayer = false;
        break;
      }

      if ((pos.x - r) <= 0) {
        asteroids[i].delta = Vector2Reflect(asteroids[i].delta, normalRight);
        asteroids[i].launchedByPlayer = false;
        break;
      }

      if ((pos.y + r) >= LEVEL_HEIGHT - 1) {
        asteroids[i].delta = Vector2Reflect(asteroids[i].delta, normalUp);
        asteroids[i].launchedByPlayer = false;
        break;
      }

      if ((pos.x + r) >= LEVEL_WIDTH - 1) {
        asteroids[i].delta = Vector2Reflect(asteroids[i].delta, normalLeft);
        asteroids[i].launchedByPlayer = false;
        break;
      }
    }
  }
}

void updateAsteroids(void) {
  for (int i = 0; i < asteroidsLen; i++) {
    for (int k = 0; k < asteroidsLen; k++) {
      if (k == i) {
        continue;
      }

      checkForCollisionsBetweenAsteroids(i, k);
    }

    asteroids[i].position = Vector2Add(asteroids[i].position, asteroids[i].delta);

    float properAngle = asteroids[i].angle + 180;
    asteroids[i].angle = mod(properAngle + asteroids[i].angleDelta, 360) - 180;

    for (int j = 0; j < asteroids[i].sprite->boundingCirclesLen; j++) {
      asteroids[i].processedBoundingCircles[j].position =
        Vector2Rotate(asteroids[i].sprite->boundingCircles[j].position,
                      asteroids[i].angle * DEG2RAD);

      asteroids[i].processedBoundingCircles[j].position =
        Vector2Scale(asteroids[i].processedBoundingCircles[j].position, SPRITES_SCALE);

      asteroids[i].processedBoundingCircles[j].radius =
        asteroids[i].sprite->boundingCircles[j].radius * SPRITES_SCALE;
    }
  }

  checkForCollisionsBetweenAsteroidsAndBorders();
}

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

float playerLookingAngle(void) {
  static const Vector2 up = {
    .x = 0,
    .y = -1,
  };

  float angle = Vector2Angle(up, lookingDirection) * RAD2DEG;

  return angle;
}

void bossMarineCheckCollisions(bool sendAsteroidsFlying) {
  for (int i = 0; i < BOSS_MARINE_BOUNDING_CIRCLES; i++) {
    Vector2 bcPos = Vector2Add(bossMarine.processedBoundingCircles[i].position,
                               bossMarine.position);
    float r = bossMarine.processedBoundingCircles[i].radius;

    for (int ai = 0; ai < asteroidsLen; ai++) {
      for (int abi = 0; abi < asteroids[ai].sprite->boundingCirclesLen; abi++) {
        Vector2 pos =
          Vector2Add(asteroids[ai].processedBoundingCircles[abi].position,
                     asteroids[ai].position);

        float distance = Vector2Distance(pos, bcPos);
        float radiusSum =
          (asteroids[ai].processedBoundingCircles[abi].radius + r);

        if (distance < radiusSum) {

          if (asteroids[ai].launchedByPlayer) {
#define ASTEROID_BASE_DAMAGE 4
            float damageMultiplier = Vector2Length(asteroids[ai].delta);
            bossMarine.health = (int)Clamp(bossMarine.health - (damageMultiplier * ASTEROID_BASE_DAMAGE),
                                           0.0f,
                                           BOSS_MARINE_MAX_HEALTH);
            asteroids[ai].launchedByPlayer = false;
          }

          float angle = angleBetweenPoints(bcPos, asteroids[ai].position) * DEG2RAD;

          float offsetDistance = distance - radiusSum;
          Vector2 asteroidOffset = Vector2Rotate((Vector2) {0, offsetDistance}, angle);

          asteroids[ai].position = Vector2Add(asteroids[ai].position, asteroidOffset);
          if (sendAsteroidsFlying) {
            asteroids[ai].delta = Vector2Add(asteroids[ai].delta, Vector2Scale(asteroidOffset, 0.5));
          }
        }
      }
    }
  }
}

void bossMarineUpdateWeapon(void) {
  Vector2 weaponOffset = (Vector2) {
    .x = bossMarine.horizontalFlip * bossMarine.weaponOffset.x,
    .y = bossMarine.weaponOffset.y,
  };

  bossMarine.weaponAngle =
    angleBetweenPoints(Vector2Add(bossMarine.position,
                                  weaponOffset),
                       player.position) - 90;
  if (bossMarine.horizontalFlip == -1) {
    bossMarine.weaponAngle += 180;
  }

  bossMarine.weaponAngle += bossMarine.weaponAngleOffset;

  Vector2 bulletOrigin = Vector2Rotate((Vector2) {
      .x = bossMarine.horizontalFlip * ((bossMarineWeaponRect.width / 2) * SPRITES_SCALE),
      .y = -6 * SPRITES_SCALE,
    }, bossMarine.weaponAngle * DEG2RAD);
  bossMarine.bulletOrigin = Vector2Add(weaponOffset, bulletOrigin);
}

void bossMarineWalk(void) {
  float playerBossAngle = angleBetweenPoints(player.position,
                                             bossMarine.position);
#define BOSS_MARINE_SPEED 4
#define BOSS_MARINE_MIN_PLAYER_DISTANCE 200
#define BOSS_MARINE_MAX_PLAYER_DISTANCE 400

  float playerBossDistance = Vector2Distance(player.position,
                                             bossMarine.position);

  Vector2 delta = Vector2Rotate((Vector2) {0, -BOSS_MARINE_SPEED},
                                playerBossAngle * DEG2RAD);

  if (playerBossDistance < BOSS_MARINE_MIN_PLAYER_DISTANCE) {
    bossMarine.position = Vector2Add(bossMarine.position, Vector2Scale(delta, 2));
  } else if (playerBossDistance > BOSS_MARINE_MAX_PLAYER_DISTANCE && bossMarine.isWalking) {
    bossMarine.position = Vector2Add(bossMarine.position, Vector2Scale(delta, -1));
  } else if (bossMarine.isWalking) {
    float angle = bossMarine.walkingDirection * BOSS_MARINE_SPEED;
    Vector2 diff = Vector2Subtract(bossMarine.position, player.position);
    diff = Vector2Rotate(diff, angle * DEG2RAD);
    bossMarine.position = Vector2Lerp(bossMarine.position, Vector2Add(diff, player.position), 0.1f);
  }
}

#define BOSS_MARINE_BASE_DAMAGE 1
#define BOSS_MARINE_PROJECTILE_RADIUS 13
#define BOSS_MARINE_PROJECTILE_SPEED 25.0f

void bossMarineShoot(float speedMultiplier,
                     float cooldown,
                     float spread,
                     Sound *sound,
                     float lifetime,
                     bool willBounce,
                     Color inside,
                     Color outside) {
  if (bossMarine.fireCooldown > 0.0f) {
    return;
  }

  Projectile *new_projectile = push_projectile();

  if (new_projectile == NULL) {
    return;
  }

  *new_projectile = (Projectile) {
    .type = PROJECTILE_REGULAR,
    .isHurtfulForBoss = false,
    .isHurtfulForPlayer = true,
    .damage = BOSS_MARINE_BASE_DAMAGE,
    .origin = Vector2Add(bossMarine.position, bossMarine.bulletOrigin),
    .radius = BOSS_MARINE_PROJECTILE_RADIUS,
    .delta = Vector2Rotate(Vector2Scale((Vector2) {bossMarine.horizontalFlip, 0}, BOSS_MARINE_PROJECTILE_SPEED * speedMultiplier),
                           (bossMarine.weaponAngle + spread) * DEG2RAD),
    .angle = 0,
    .inside = inside,
    .outside = outside,
    .lifetime = lifetime,
    .canBounce = willBounce,
  };
  bossMarine.fireCooldown = cooldown;

  if (sound) {
    PlaySound(*sound);
  }
}

static bool bossMarineAttacks[BOSS_MARINE_NOT_SHOOTING] = {0};

void bossMarineAttack(void) {
  bossMarine.attackTimer -= GetFrameTime();
  bossMarine.fireCooldown -= GetFrameTime();

  if (bossMarine.attackTimer <= 0.0f) {
    bossMarine.isWalking = (bool)GetRandomValue(0, 1);

    if (bossMarine.currentAttack == BOSS_MARINE_NOT_SHOOTING) {
      int c = 0;
      for (int i = 0; i < BOSS_MARINE_NOT_SHOOTING; i++) {
        if (bossMarineAttacks[i]) {
          c++;
        }
      }

      if (c == BOSS_MARINE_NOT_SHOOTING) {
        memset(bossMarineAttacks, 0, sizeof(bossMarineAttacks));
      }

      BossMarineAttack a = 0;
      do {
        a = GetRandomValue(BOSS_MARINE_SINUS_SHOOTING,
                           BOSS_MARINE_BOUNCING_WAVES);
      } while (bossMarineAttacks[a]);
      bossMarineAttacks[a] = true;

      bossMarine.currentAttack = a;

      bossMarine.attackTimer = (float)GetRandomValue(3, 7);
    } else {
      bossMarine.currentAttack = BOSS_MARINE_NOT_SHOOTING;
      bossMarine.attackTimer = (float)GetRandomValue(1, 10) / 10.0f;
    }

    bossMarine.weaponAngleOffset = 0;
  }

  switch (bossMarine.currentAttack) {
  case BOSS_MARINE_SINUS_SHOOTING: {
    bossMarine.weaponAngleOffset = sin(time * 4) * 30 * bossMarine.horizontalFlip;
    bossMarineUpdateWeapon();

    bossMarineShoot(0.3f, 0.04f, 0, &bossMarineGunshotSound, 10, false, MAROON, GOLD);
  } break;
  case BOSS_MARINE_NOT_SHOOTING: {
    bossMarineUpdateWeapon();
  } break;
  case BOSS_MARINE_SHOOTING: {
    bossMarineUpdateWeapon();
    bossMarineShoot(0.5f, 0.06f, (float)GetRandomValue(-30, 30), &bossMarineGunshotSound, 10, false, MAROON, GOLD);
  } break;
  case BOSS_MARINE_SHOTGUNNING: {
    bossMarineUpdateWeapon();

    if (bossMarine.fireCooldown > 0.0f) {
      break;
    }

    for (int i = 0; i < 30; i++) {
      (void) i;

      float spread = (float)GetRandomValue(-30, 30);
      float speed = (float)GetRandomValue(1, 10) / 10.0f * 0.8f;

      bossMarineShoot(speed,
                      0,
                      spread,
                      NULL,
                      5.0f,
                      false,
                      MAROON,
                      GOLD);
    }

    PlaySound(bossMarineShotgunSound);
    bossMarine.fireCooldown = (float)GetRandomValue(5, 10) / 10.0f;

  } break;
  case BOSS_MARINE_BOUNCING_WAVES: {
    bossMarineUpdateWeapon();

    if (bossMarine.fireCooldown > 0.0f) {
      break;
    }

    for (int i = -40; i < 40; i += 2) {
      float spread = (float)i;

      bossMarineShoot(0.7f,
                      0,
                      spread,
                      NULL,
                      4.0f,
                      true,
                      GOLD,
                      MAROON);
    }

    PlaySound(bossMarineShotgunSound);
    bossMarine.fireCooldown = (float)GetRandomValue(7, 10) / 10.0f;
  } break;
  };
}

void updateBossMarine(void) {
  if (bossMarine.health <= 0) {
    PauseMusicStream(bossMarineMusic);
    playerStats.kills += 1;
    gameState = GAME_BOSS_DEAD;

    for (int i = 0; i < PROJECTILES_MAX; i++) {
      if (projectiles[i].type != PROJECTILE_NONE) {
        projectiles[i].willBeDestroyed = true;
        projectiles[i].destructionTimer = 0.02f;
      }
    }
    return;
  }

  bossMarine.horizontalFlip =
    (player.position.x < bossMarine.position.x)
    ? -1
    : 1;

  for (int i = 0; i < BOSS_MARINE_BOUNDING_CIRCLES; i++) {
    bossMarine.processedBoundingCircles[i] = bossMarine.boundingCircles[i];
    bossMarine.processedBoundingCircles[i].position.x *= bossMarine.horizontalFlip;
  }

  bossMarineCheckCollisions(true);
  bossMarineWalk();

  bossMarineAttack();

  Vector2 minPos = Vector2Scale((Vector2){bossMarineRect.width / 2, bossMarineRect.width / 2},
                                SPRITES_SCALE);
  Vector2 maxPos = Vector2Subtract((Vector2) {LEVEL_WIDTH, LEVEL_HEIGHT},
                                   minPos);
  bossMarine.position = Vector2Clamp(bossMarine.position, minPos, maxPos);
}

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
    isGamePaused = true;
#ifndef PLATFORM_WEB
    DisableCursor();
#endif
  }

#ifdef PLATFORM_DESKTOP

  if (!IsWindowFocused() || IsWindowHidden()) {
    isGamePaused = true;
  }

#endif

  lookingDirection = Vector2Normalize(Vector2Subtract(mouseCursor, player.position));
}

#define PLAYER_DASH_DISTANCE 20

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
#define PLAYER_PROJECTILE_SPEED 30.0f
#define PLAYER_PROJECTILE_BASE_DAMAGE 4

void tryFiringAShot(void) {
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
      player.fireCooldown > 0.0f ||
      player.dashCooldown > 0.0f) {
    return;
  }

  Projectile *new_projectile = push_projectile();

  if (new_projectile == NULL) {
    return;
  }

  int halfSpread = player.bulletSpread / 2;
  int a = GetRandomValue(-halfSpread, halfSpread);

  *new_projectile = (Projectile) {
    .type = PROJECTILE_SQUARED,
    /* .type = PROJECTILE_REGULAR, */

    .isHurtfulForPlayer = false,
    .isHurtfulForBoss = true,

    .damage = PLAYER_PROJECTILE_BASE_DAMAGE,

    .origin = Vector2Add(player.position, Vector2Scale(lookingDirection, 35)),
    /* .radius = PLAYER_PROJECTILE_RADIUS, */
    .size = (Vector2) {
      .x = PLAYER_PROJECTILE_RADIUS * 1.5,
      .y = PLAYER_PROJECTILE_RADIUS * 3,
    },
    .delta = Vector2Rotate(Vector2Scale(lookingDirection, PLAYER_PROJECTILE_SPEED),
                           a * DEG2RAD),
    .angle = playerLookingAngle() + (float)a,

    .inside = DARKBLUE,
    .outside = BLUE,
    .lifetime = 10,
    .canBounce = false,
  };

  player.fireCooldown = PLAYER_FIRE_COOLDOWN;

  PlaySound(playerShot);
}

typedef enum {
  KEY_MOVE_UP,
  KEY_MOVE_LEFT,
  KEY_MOVE_DOWN,
  KEY_MOVE_RIGHT,
  KEY_MOVE_COUNT,
} MovementKeys;

static KeyboardKey esdfKeys[KEY_MOVE_COUNT] = {
  [KEY_MOVE_UP] = KEY_E,
  [KEY_MOVE_LEFT] = KEY_S,
  [KEY_MOVE_DOWN] = KEY_D,
  [KEY_MOVE_RIGHT] = KEY_F,
};

static KeyboardKey wasdKeys[KEY_MOVE_COUNT] = {
  [KEY_MOVE_UP] = KEY_W,
  [KEY_MOVE_LEFT] = KEY_A,
  [KEY_MOVE_DOWN] = KEY_S,
  [KEY_MOVE_RIGHT] = KEY_D,
};

void movePlayerWithAKeyboard(void) {
  KeyboardKey *keys = wasdKeys;

  if (esdf) {
    keys = esdfKeys;
  }

  if (IsKeyDown(keys[KEY_MOVE_UP])) {
    player.movementDirection |= DIRECTION_UP;
    player.movementDelta.y -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(keys[KEY_MOVE_LEFT])) {
    player.movementDirection |= DIRECTION_LEFT;
    player.movementDelta.x -= PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(keys[KEY_MOVE_DOWN])) {
    player.movementDirection |= DIRECTION_DOWN;
    player.movementDelta.y += PLAYER_MOVEMENT_SPEED;
  }

  if (IsKeyDown(keys[KEY_MOVE_RIGHT])) {
    player.movementDirection |= DIRECTION_RIGHT;
    player.movementDelta.x += PLAYER_MOVEMENT_SPEED;
  }

  player.position = Vector2Add(player.position, player.movementDelta);
}

typedef struct {
  Vector2 position;
  float angle;
  float alpha;
} PlayerDashTrail;

#define PLAYER_DASH_TRAILS_MAX 64
static PlayerDashTrail dashTrails[PLAYER_DASH_TRAILS_MAX] = {0};

PlayerDashTrail *pushDashTrail(void) {
  for (int i = 0; i < PLAYER_DASH_TRAILS_MAX; i++) {
    if (dashTrails[i].alpha <= 0.0f) {
      return &dashTrails[i];
    }
  }

  return NULL;
}

void movePlayerWithADash(void) {
#define DASH_DELTA_LERP_RATE 0.12f
  player.dashDelta = Vector2Lerp(player.dashDelta,
                                 Vector2Zero(),
                                 DASH_DELTA_LERP_RATE);

  player.isInvincible = (roundf(player.dashDelta.x) != 0.0f ||
                         roundf(player.dashDelta.y) != 0.0f);

  player.position =
    Vector2Add(player.position,
               player.dashDelta);

  if (!player.isInvincible) {
    return;
  }

  PlayerDashTrail *new_trail = pushDashTrail();
  if (!new_trail) {
    return;
  }

  *new_trail = (PlayerDashTrail) {
    .position = player.position,
    .angle = playerLookingAngle(),
    .alpha = 1.0f,
  };
}

void updatePlayerDashTrails(void) {
  for (int i = 0; i < PLAYER_DASH_TRAILS_MAX; i++) {
    dashTrails[i].alpha = Clamp(dashTrails[i].alpha - (GetFrameTime() * 3),
                                0.0f,
                                1.0f);
  }
}

void processCollisions(void) {
  for (int i = 0; i < asteroidsLen; i++) {
    for (int j = 0; j < asteroids[i].sprite->boundingCirclesLen; j++) {
      Vector2 pos =
        Vector2Add(asteroids[i].processedBoundingCircles[j].position,
                   asteroids[i].position);

      float distance = Vector2Distance(pos, player.position);
      float radiusSum =
        (asteroids[i].processedBoundingCircles[j].radius + PLAYER_HITBOX_RADIUS);

      if (distance < radiusSum) {
        float angle = angleBetweenPoints(pos, player.position);

        if (player.isInvincible) {
          angle += 180;
        }

        float offsetDistance = distance - radiusSum;
        Vector2 offset = Vector2Rotate((Vector2) {0, offsetDistance}, angle * DEG2RAD);

        if (player.isInvincible) {
          asteroids[i].position = Vector2Add(asteroids[i].position, offset);
          asteroids[i].delta = Vector2Add(asteroids[i].delta, Vector2Scale(offset, 0.2f));
          asteroids[i].launchedByPlayer = true;
        } else {
          player.position = Vector2Add(player.position, offset);
        }
      }
    }
  }
}

void updatePlayerPosition(void) {
  player.movementDirection = 0;
  player.movementDelta = Vector2Zero();

  movePlayerWithAKeyboard();
  movePlayerWithADash();

  processCollisions();

  player.position =
    Vector2Clamp(player.position,
                 (Vector2) {
                   .x = PLAYER_HITBOX_RADIUS,
                   .y = PLAYER_HITBOX_RADIUS,
                 },
                 (Vector2) {
                   .x = level.x - PLAYER_HITBOX_RADIUS,
                   .y = level.y - PLAYER_HITBOX_RADIUS,
                 });
}

void renderBackground(void) {
  Vector2 pos = player.position;

  if (gameState == GAME_MAIN_MENU) {
    pos = mouseCursor;
  }

  float background_x = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            pos.x / LEVEL_WIDTH);

  float background_y = Lerp(0, BACKGROUND_PARALLAX_OFFSET,
                            pos.y / LEVEL_HEIGHT);

  SetShaderValue(stars, starsTime, &time, SHADER_UNIFORM_FLOAT);

  BeginBlendMode(BLEND_ALPHA); {
    ClearBackground((Color) {41, 1, 53, 69});
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
static RenderTexture2D playerTexture1 = {0};

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

  BeginTextureMode(playerTexture1); {
    ClearBackground(BLANK);

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

    float health = (float)player.health / (float)MAX_PLAYER_HEALTH;

    SetShaderValue(playerHealthOverlayShader,
                   playerHealthOverlayHealth,
                   &health,
                   SHADER_UNIFORM_FLOAT);

    BeginShaderMode(playerHealthOverlayShader); {
      DrawTexture(playerHealthMaskTexture,
                  1, 1,
                  WHITE);
    } EndShaderMode();
  } EndTextureMode();

  BeginTextureMode(playerTexture); {
    ClearBackground(BLANK);

    SetShaderValue(dashResetShader,
                   dashResetShaderAlpha,
                   &player.dashReactivationEffectAlpha,
                   SHADER_UNIFORM_FLOAT);

    BeginShaderMode(dashResetShader); {
      DrawTexture(playerTexture1.texture,
                  0, 0,
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

void renderPlayer(void) {
  float alpha = 1.0;

  if (player.iframeTimer > 0.0f) {
    float a = sinf(time * 40) * .5 + 1.;
    alpha = Remap(a,
                  0, 1,
                  0.7, 1.0);
  }

  DrawTexturePro(playerTexture.texture,
                 (Rectangle) {
                   .x = 0,
                   .y = 0,
                   .width = playerTexture.texture.width,
                   .height = playerTexture.texture.height,
                 },
                 (Rectangle) {
                   .x = player.position.x,
                   .y = player.position.y,
                   .width = playerTexture.texture.width * SPRITES_SCALE,
                   .height = playerTexture.texture.height * SPRITES_SCALE,
                 },
                 (Vector2) {
                   .x = (playerRect.width * SPRITES_SCALE) / 2,
                   .y = (playerRect.height * SPRITES_SCALE) / 2,
                 },
                 playerLookingAngle(),
                 ColorAlpha(WHITE, alpha));
}

void renderDashTrails(void) {
  for (int i = 0; i < PLAYER_DASH_TRAILS_MAX; i++) {
    if (dashTrails[i].alpha <= 0) {
      continue;
    }

    float w = playerRect.width * SPRITES_SCALE;
    float h = playerRect.height * SPRITES_SCALE;

    SetShaderValue(dashTrailShader,
                   dashTrailShaderAlpha,
                   &dashTrails[i].alpha,
                   SHADER_UNIFORM_FLOAT);

    BeginShaderMode(dashTrailShader); {
      DrawTexturePro(sprites,
                     playerRect,
                     (Rectangle) {
                       .x = dashTrails[i].position.x,
                       .y = dashTrails[i].position.y,
                       .width = w,
                       .height = h,
                     },
                     (Vector2) {w / 2, h / 2},
                     dashTrails[i].angle,
                     WHITE);
    }; EndShaderMode();
  }
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
                     .width = thrusterTrail[i].texture.texture.width * SPRITES_SCALE,
                     .height = thrusterTrail[i].texture.texture.height * SPRITES_SCALE,
                   },
                   (Vector2) {
                     .x = (playerRect.width * SPRITES_SCALE) / 2,
                     .y = (playerRect.height * SPRITES_SCALE) / 2,
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

#define MOUSE_CURSOR_SCALE (SPRITES_SCALE)
void renderMouseCursor(void) {
  DrawTexturePro(sprites,
                 mouseCursorRect,
                 (Rectangle) {
                   .x = screenMouseLocation.x,
                   .y = screenMouseLocation.y,
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
    float radiusScale = projectiles[i].willBeDestroyed ? 1.5f : 1.0f;

    switch (projectiles[i].type) {
    case PROJECTILE_NONE: break;
    case PROJECTILE_REGULAR: {
      DrawCircleV(projectiles[i].origin,
                  projectiles[i].radius * radiusScale,
                  projectiles[i].outside);

      if (projectiles[i].willBeDestroyed) {
        break;
      }

      DrawCircleV(projectiles[i].origin,
                  projectiles[i].radius - PROJECTILE_BORDER,
                  projectiles[i].inside);
    } break;
    case PROJECTILE_SQUARED: {
      Rectangle shape = (Rectangle) {
        .x = projectiles[i].origin.x,
        .y = projectiles[i].origin.y,
        .width = projectiles[i].size.x * radiusScale,
        .height = projectiles[i].size.y * radiusScale,
      };

      DrawRectanglePro(shape,
                       (Vector2) {
                         .x = shape.width / 2,
                         .y = shape.height / 2,
                       },
                       projectiles[i].angle,
                       projectiles[i].outside);

      if (projectiles[i].willBeDestroyed) {
        break;
      }

      shape.width -= PROJECTILE_BORDER * 2;
      shape.height -= PROJECTILE_BORDER * 2;

      DrawRectanglePro(shape,
                       (Vector2) {
                         .x = shape.width / 2,
                         .y = shape.height / 2,
                       },
                       projectiles[i].angle,
                       projectiles[i].inside);
    } break;
    }
  }
}

Vector2 arenaTopLeft = {0};
Vector2 arenaBottomRight = {0};

void renderArenaBorder(void) {
  SetShaderValue(arenaBorderShader,
                 arenaBorderTime,
                 &time,
                 SHADER_UNIFORM_FLOAT);

  Vector2 arenaSize = Vector2Subtract(arenaBottomRight, arenaTopLeft);
  BeginShaderMode(arenaBorderShader); {
    DrawRectangleV(arenaTopLeft,
                   arenaSize,
                   BLUE);
  } EndShaderMode();
}

void renderAsteroids(void) {
  for (int i = 0; i < asteroidsLen; i++) {
    Vector2 center = {
      .x = (asteroids[i].sprite->textureRect.width * SPRITES_SCALE) / 2,
      .y = (asteroids[i].sprite->textureRect.height * SPRITES_SCALE) / 2,
    };

    DrawTexturePro(sprites,
                   asteroids[i].sprite->textureRect,
                   (Rectangle) {
                     .x = asteroids[i].position.x,
                     .y = asteroids[i].position.y,
                     .width = asteroids[i].sprite->textureRect.width * SPRITES_SCALE,
                     .height = asteroids[i].sprite->textureRect.height * SPRITES_SCALE,
                   },
                   center,
                   asteroids[i].angle,
                   WHITE);
  }
}

void renderBackgroundAsteroid(void) {
  Vector2 center = {
    .x = (bigAssAsteroidRect.width * BIG_ASS_ASTEROID_SCALE) / 2,
    .y = (bigAssAsteroidRect.height * BIG_ASS_ASTEROID_SCALE) / 2,
  };

  DrawTexturePro(sprites,
                 bigAssAsteroidRect,
                 (Rectangle) {
                   .x = bigAssAsteroidPosition.x,
                   .y = bigAssAsteroidPosition.y,
                   .width = bigAssAsteroidRect.width * BIG_ASS_ASTEROID_SCALE,
                   .height = bigAssAsteroidRect.height * BIG_ASS_ASTEROID_SCALE,
                 },
                 center,
                 bigAssAsteroidAngle,
                 GRAY);
}

void renderBoss(void) {
  Vector2 center = {
    .x = (bossMarineRect.width * SPRITES_SCALE) / 2,
    .y = (bossMarineRect.height * SPRITES_SCALE) / 2,
  };

  Rectangle bossRect = bossMarineRect;
  bossRect.width *= bossMarine.horizontalFlip;
  Rectangle weaponRect = bossMarineWeaponRect;
  weaponRect.width *= bossMarine.horizontalFlip;

  DrawTexturePro(sprites,
                 bossRect,
                 (Rectangle) {
                   .x = bossMarine.position.x,
                   .y = bossMarine.position.y,
                   .width = bossMarineRect.width * SPRITES_SCALE,
                   .height = bossMarineRect.height * SPRITES_SCALE,
                 },
                 center,
                 0,
                 WHITE);

  Vector2 weaponCenter = {
    .x = (bossMarineWeaponRect.width * SPRITES_SCALE) / 2,
    .y = (bossMarineWeaponRect.height * SPRITES_SCALE) / 2,
  };

  DrawTexturePro(sprites,
                 weaponRect,
                 (Rectangle) {
                   .x = bossMarine.position.x + (bossMarine.weaponOffset.x * bossMarine.horizontalFlip),
                   .y = bossMarine.position.y + bossMarine.weaponOffset.y,
                   .width = bossMarineWeaponRect.width * SPRITES_SCALE,
                   .height = bossMarineWeaponRect.height * SPRITES_SCALE,
                 },
                 weaponCenter,
                 bossMarine.weaponAngle,
                 WHITE);
}

static float blackBackgroundAlpha = 0;

void renderPhase1(void) {
  renderPlayerTexture();

  BeginTextureMode(target); {
    ClearBackground(BLACK);

    renderBackground();
    renderBackgroundAsteroid();

    if (gameState == GAME_BOSS ||
        (gameState == GAME_BOSS_INTRODUCTION &&
         introductionStage != BOSS_INTRODUCTION_BEGINNING)) {
      renderArenaBorder();
    }

    renderAsteroids();

    if (gameState == GAME_BOSS_DEAD || gameState == GAME_PLAYER_DEAD) {
      DrawRectangleV(Vector2Zero(),
                     level,
                     ColorAlpha(BLACK, blackBackgroundAlpha));
    }

    renderBoss();

    renderThrusterTrails();
    renderDashTrails();
    renderPlayer();

    renderProjectiles();

  } EndTextureMode();
}

#define HEALTH_BAR_HEIGHT 5

static Rectangle bossMarineHeadRect = {
  .x = 76,
  .y = 30,
  .width = 23,
  .height = 21,
};

void renderBossHealthBar(void) {
  const float screenFill = 0.7;

  const float width = (float)GetScreenWidth() * screenFill;
  const float height = HEALTH_BAR_HEIGHT * SPRITES_SCALE * camera.zoom;

  Rectangle rect = {
    .x = (float)GetScreenWidth() / 2,
    .y = height * 1.5,
    .width = width,
    .height = height,
  };

  Rectangle wRect = rect;
  wRect.width += SPRITES_SCALE * camera.zoom * 2;

  Rectangle hRect = rect;
  hRect.height += SPRITES_SCALE * camera.zoom * 2;

  DrawRectanglePro(wRect, (Vector2) {wRect.width / 2, wRect.height / 2}, 0, BLACK);
  DrawRectanglePro(hRect, (Vector2) {hRect.width / 2, hRect.height / 2}, 0, BLACK);

  float health = (float)bossMarine.health / BOSS_MARINE_MAX_HEALTH;

  rect.x -= rect.width / 2;

  Vector2 head = {
    .x = rect.x + rect.width * health,
    .y = rect.y,
  };

  rect.y -= rect.height / 2;


  rect.width *= health;

  DrawRectanglePro(rect, Vector2Zero(), 0, RED);

  float mul = SPRITES_SCALE * camera.zoom * 0.6;

  Font f = GetFontDefault();
  char *bossName = BOSS_MARINE_NAME;
  float fontSize = 11 * mul;
  float spacing = 1 * mul;
  Vector2 size = MeasureTextEx(f, bossName, fontSize, spacing);

  Vector2 pos = {
      .x = (float)GetScreenWidth() / 2,
      .y = height * 2,
  };

  DrawTextPro(f, bossName, pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, BLACK);
  DrawTextPro(f, bossName, Vector2SubtractValue(pos, spacing), Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);

  DrawTexturePro(sprites,
                 bossMarineHeadRect,
                 (Rectangle) {
                   .x = head.x,
                   .y = head.y,
                   .width = bossMarineHeadRect.width * mul,
                   .height = bossMarineHeadRect.height * mul,
                 },
                 (Vector2) {
                   .x = (bossMarineHeadRect.width * mul) / 2,
                   .y = (bossMarineHeadRect.height * mul) / 2,
                 },
                 sin(time * 2) * 15,
                 WHITE);
}

static Vector2 bossInfoHeadPosition = {0};
static float infoXBase = 0;

void renderBossInfo(void) {
  float w = GetScreenWidth();
  float h = GetScreenHeight();

  Color red = (Color) {143, 30, 32, 255};

  float w3 = roundf(w / 3);

  float x1 = ceilf(infoXBase + w3);
  float x2 = floorf(x1 + w3);

  DrawRectangle(infoXBase, 0,
                w3, h,
                red);

  DrawTriangle((Vector2) {x1, 0},
               (Vector2) {x1, h},
               (Vector2) {x2, h},
               red);

  float mul = 25;

  DrawTexturePro(sprites,
                 bossMarineHeadRect,
                 (Rectangle) {
                   .x = bossInfoHeadPosition.x,
                   .y = bossInfoHeadPosition.y,
                   .width = bossMarineHeadRect.width * mul,
                   .height = bossMarineHeadRect.height * mul,
                 },
                 (Vector2) {
                   .x = (bossMarineHeadRect.width * mul) / 2,
                   .y = (bossMarineHeadRect.height * mul) / 2,
                 },
                 sin(time) * 15,
                 WHITE);

  Font f = GetFontDefault();
  char *bossName = BOSS_MARINE_NAME;
  float fontSize = 11 * mul / 4;
  float spacing = 1 * mul / 4;
  Vector2 size = MeasureTextEx(f, bossName, fontSize, spacing);

  Vector2 pos = (Vector2) {
      .x = w / 4 * 3,
      .y = h / 2,
  };

  DrawTextPro(f, bossName, Vector2AddValue(pos, spacing),
              Vector2Scale(size, 0.5f),
              0,
              fontSize,
              spacing,
              red);

  DrawTextPro(f, bossName, pos,
              Vector2Scale(size, 0.5f),
              0,
              fontSize,
              spacing,
              WHITE);
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

    if (gameState == GAME_BOSS_INTRODUCTION &&
        introductionStage == BOSS_INTRODUCTION_INFO) {
      renderBossInfo();
    }

    if (gameState == GAME_BOSS) {
      renderBossHealthBar();
    }

    if (gameState != GAME_BOSS_INTRODUCTION &&
        gameState != GAME_PLAYER_DEAD &&
        gameState != GAME_BOSS_DEAD &&
        !isGamePaused) {
      renderMouseCursor();
    }

  } EndDrawing();
}

bool doesRectangleCollideWithACircle(Rectangle a, float angle,
                                     Vector2 b, float r) {
  Vector2 aPos = {a.x, a.y};
  Vector2 relBPos = Vector2Subtract(b, aPos);
  Vector2 rotRelBPos = Vector2Rotate(relBPos, (-angle) * DEG2RAD);
  Vector2 rotBPos = Vector2Add(aPos, rotRelBPos);

  return CheckCollisionCircleRec(rotBPos, r, a);
}

void checkRegularProjectileCollision(int i) {
  Vector2 proj = projectiles[i].origin;
  float radius = projectiles[i].radius;

  for (int j = 0; j < asteroidsLen; j++) {
    for (int bj = 0; bj < asteroids[j].sprite->boundingCirclesLen; bj++) {
      Vector2 pos = Vector2Add(asteroids[j].processedBoundingCircles[bj].position,
                               asteroids[j].position);
      float r = asteroids[j].processedBoundingCircles[bj].radius;

      if (CheckCollisionCircles(proj, radius, pos, r)) {
        projectiles[i].willBeDestroyed = true;
        return;
      }
    }
  }

  if (!player.isInvincible &&
      !projectiles[i].willBeDestroyed &&
      projectiles[i].isHurtfulForPlayer &&
      CheckCollisionCircles(proj, radius, player.position, PLAYER_HITBOX_RADIUS)) {

    projectiles[i].willBeDestroyed = true;

    if (player.iframeTimer == 0.0f) {
      PlaySound(hit);
      player.health -= projectiles[i].damage;
      player.iframeTimer = 0.3f;
    }

    if (player.health == 0) {
      gameState = GAME_PLAYER_DEAD;
      PauseMusicStream(bossMarineMusic);
      PlaySound(playerDeathSound);
    }
    return;
  }

  if (!projectiles[i].isHurtfulForBoss) {
    return;
  }

  for (int j = 0; j < BOSS_MARINE_BOUNDING_CIRCLES; j++) {
    Vector2 pos = Vector2Add(bossMarine.position,
                             bossMarine.processedBoundingCircles[j].position);
    float r = bossMarine.processedBoundingCircles[j].radius;

    if (CheckCollisionCircles(proj, radius, pos, r)) {
      projectiles[i].willBeDestroyed = true;
      bossMarine.health -= projectiles[i].damage;
      return;
    }
  }
}

void checkSquaredProjectileCollision(int i) {
  Rectangle proj = {
    .x = projectiles[i].origin.x - (projectiles[i].size.x / 2),
    .y = projectiles[i].origin.y - (projectiles[i].size.y / 2),
  };
  float angle = projectiles[i].angle;

  for (int j = 0; j < asteroidsLen; j++) {
    for (int bj = 0; bj < asteroids[j].sprite->boundingCirclesLen; bj++) {
      Vector2 pos = Vector2Add(asteroids[j].processedBoundingCircles[bj].position,
                               asteroids[j].position);
      float r = asteroids[j].processedBoundingCircles[bj].radius;

      if (doesRectangleCollideWithACircle(proj, angle,
                                          pos, r)) {
        projectiles[i].willBeDestroyed = true;
        return;
      }
    }
  }

  if (projectiles[i].isHurtfulForPlayer &&
      doesRectangleCollideWithACircle(proj, angle,
                                      player.position, PLAYER_HITBOX_RADIUS)) {
    projectiles[i].willBeDestroyed = true;
    player.health -= projectiles[i].damage;
    return;
  }

  if (!projectiles[i].isHurtfulForBoss) {
    return;
  }

  for (int j = 0; j < BOSS_MARINE_BOUNDING_CIRCLES; j++) {
    Vector2 pos = Vector2Add(bossMarine.position,
                             bossMarine.processedBoundingCircles[j].position);
    float r = bossMarine.processedBoundingCircles[j].radius;

    if (doesRectangleCollideWithACircle(proj, angle,
                                        pos, r)) {
      projectiles[i].willBeDestroyed = true;
      bossMarine.health -= projectiles[i].damage;
      return;
    }
  }
}

void updateProjectiles(void) {
  Vector2 normalDown = {0, 1};
  Vector2 normalUp = {0, -1};
  Vector2 normalRight = {1, 0};
  Vector2 normalLeft = {-1, 0};

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

    projectiles[i].lifetime -= GetFrameTime();

    if (projectiles[i].lifetime <= 0.0f) {
      projectiles[i].willBeDestroyed = true;
      projectiles[i].destructionTimer = 0.02f;
      continue;
    }

    if (projectiles[i].canBounce && projectiles[i].type == PROJECTILE_REGULAR) {
      float r = projectiles[i].radius;
      Vector2 o = projectiles[i].origin;

      #define PROJECTILE_LIFETIME_AFTER_BOUNCE 0.25f
      if ((o.x - r) <= 0) {
        projectiles[i].delta = Vector2Reflect(projectiles[i].delta,
                                              normalRight);
        projectiles[i].lifetime = MIN(PROJECTILE_LIFETIME_AFTER_BOUNCE, projectiles[i].lifetime);
      }

      if ((o.x + r) >= LEVEL_WIDTH - 1) {
        projectiles[i].delta = Vector2Reflect(projectiles[i].delta,
                                              normalLeft);
        projectiles[i].lifetime = MIN(PROJECTILE_LIFETIME_AFTER_BOUNCE, projectiles[i].lifetime);
      }

      if ((o.y - r) <= 0) {
        projectiles[i].delta = Vector2Reflect(projectiles[i].delta,
                                              normalDown);
        projectiles[i].lifetime = MIN(PROJECTILE_LIFETIME_AFTER_BOUNCE, projectiles[i].lifetime);
      }

      if ((o.y + r) >= LEVEL_HEIGHT - 1) {
        projectiles[i].delta = Vector2Reflect(projectiles[i].delta,
                                              normalUp);
        projectiles[i].lifetime = MIN(PROJECTILE_LIFETIME_AFTER_BOUNCE, projectiles[i].lifetime);
      }
    } else if ((projectiles[i].origin.x <= 0) ||
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
      continue;
    }

    switch (projectiles[i].type) {
    case PROJECTILE_REGULAR: checkRegularProjectileCollision(i); break;
    case PROJECTILE_SQUARED: checkSquaredProjectileCollision(i); break;
    case PROJECTILE_NONE: break;
    }

    if (projectiles[i].willBeDestroyed) {
      continue;
    }

    projectiles[i].origin = Vector2Add(projectiles[i].delta, projectiles[i].origin);
  }
}

void updatePlayerCooldowns(void) {
  float frameTime = GetFrameTime();

  bool dashCooldownActive = player.dashCooldown > 0.0f;

#define COOLDOWN_MAX 10.0f
#define DECREASE_COOLDOWN(c) (c) = Clamp((c) - frameTime, 0.0f, COOLDOWN_MAX)

  DECREASE_COOLDOWN(player.fireCooldown);
  DECREASE_COOLDOWN(player.dashCooldown);
  DECREASE_COOLDOWN(player.iframeTimer);

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

  float halfScreenWidth = (windowWidth / (2 * camera.zoom));
  float halfScreenHeight = (windowHeight / (2 * camera.zoom));

  Vector2 target = player.position;

  Vector2 topLeft = {
    halfScreenWidth,
    halfScreenHeight,
  };

  Vector2 bottomRight = {
    (float)LEVEL_WIDTH - halfScreenWidth,
    (float)LEVEL_HEIGHT - halfScreenHeight,
  };

  if (gameState == GAME_BOSS_DEAD) {
    target = bossMarine.position;
    topLeft = Vector2Zero();
    bottomRight = level;
  }

  camera.offset.x = windowWidth / 2.0f;
  camera.offset.y = windowHeight / 2.0f;

  camera.zoom = MIN(x, y);

  if (gameState == GAME_MAIN_MENU) {
    camera.target.x = (float)LEVEL_WIDTH / 2;
    camera.target.y = (float)LEVEL_HEIGHT / 2;
    return;
  }

  if (gameState == GAME_BOSS_INTRODUCTION) {
    camera.target = Vector2Clamp(cameraIntroductionTarget,
                                 topLeft,
                                 bottomRight);
    return;
  }

  target = Vector2Clamp(target,
                        topLeft,
                        bottomRight);
  camera.target = Vector2Lerp(camera.target, target, 0.1f);
}

void initRaylib() {
#if !defined(_DEBUG)
  SetTraceLogLevel(LOG_NONE);
#endif
  InitWindow(screenWidth, screenHeight, "stribun");
  InitAudioDevice();

#if defined(PLATFORM_DESKTOP)
  SetExitKey(KEY_NULL);
  SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED | FLAG_BORDERLESS_WINDOWED_MODE);
#endif
}

void initBackgroundAsteroid(void) {
  bigAssAsteroidPosition.x = (float) GetRandomValue(0, LEVEL_WIDTH);
  bigAssAsteroidPosition.y = (float) GetRandomValue(0, LEVEL_HEIGHT);

  bigAssAsteroidAngle = (float) GetRandomValue(0, 360);

  bigAssAsteroidPositionDelta.x = (float)GetRandomValue(-1, 1) * 0.05f;
  bigAssAsteroidPositionDelta.y = (float)GetRandomValue(-1, 1) * 0.05f;

  bigAssAsteroidAngleDelta = (float)GetRandomValue(-1, 1) * 0.005f;
}

void initBossMarine(void) {
  currentBoss = BOSS_MARINE;

  memset(&bossMarine, 0, sizeof(bossMarine));
  memset(bossMarineAttacks, 0, sizeof(bossMarineAttacks));

  bossMarine = (BossMarine) {
    .position = {
      .x = (float)LEVEL_WIDTH / 2,
      .y = (float)LEVEL_HEIGHT / 2,
    },
    .health = BOSS_MARINE_MAX_HEALTH,
    .boundingCircles = {
      {{4, -25}, 12},
      {{25, -14}, 5},
      {{2, 22}, 15},
      {{0, 12}, 14},
      {{7, 10}, 12},
      {{13, 0}, 7},
      {{-14, 2}, 8},
      {{-23, -9}, 12},
      {{-10, -8}, 14},
      {{6, -8}, 8},
      {{17, -11}, 9},
      {{27, -5}, 8},
    },
    .weaponAngle = 0,
    .horizontalFlip = 1,
    .walkingDirection = 1,
    .weaponOffset = bossMarineInitialWeaponOffset,
  };

  for (int i = 0; i < BOSS_MARINE_BOUNDING_CIRCLES; i++) {
    bossMarine.boundingCircles[i].position =
      Vector2Scale(bossMarine.boundingCircles[i].position,
                   SPRITES_SCALE);
    bossMarine.boundingCircles[i].radius *= SPRITES_SCALE;
  }

  bossMarineCheckCollisions(false);
};

void initMusic(void) {
  mainMenuMusic = LoadMusicStream("resources/drozerix_-_stardust_jam.mod");
  SetMusicVolume(mainMenuMusic, 0.8);

  bossMarineMusic = LoadMusicStream("resources/once_is_not_enough.mod");
  SetMusicVolume(bossMarineMusic, 0.5);
}

void initMouse(void) {
  DisableCursor();

  screenMouseLocation = (Vector2) {
    .x = (float)screenWidth / 2,
    .y = ((float)screenHeight / 6),
  };
}

void initPlayer(void) {
  memset(&player, 0, sizeof(player));
  memset(&playerStats, 0, sizeof(playerStats));

  memset(&dashTrails, 0, sizeof(dashTrails));

  player = (Player) {
    .position = (Vector2) {
      .x = (float)LEVEL_WIDTH / 2,
      .y = LEVEL_HEIGHT + ((float)LEVEL_HEIGHT / 6),
    },
    .isInvincible = false,
    .health = MAX_PLAYER_HEALTH,
    .bulletSpread = 1,
  };
}

void initCamera(void) {
  camera.rotation = 0;
  camera.zoom = 1;
}

void initProjectiles(void) {
  memset(projectiles, 0, sizeof(projectiles));
}

void initAsteroids(void) {
  asteroidsLen = GetRandomValue(MIN_ASTEROIDS, MAX_ASTEROIDS - 1);

  const int maxAsteroidSprites = (sizeof(asteroidSprites) / sizeof(asteroidSprites[0]));

  for (int i = 0; i < asteroidsLen; i++) {
    int asteroidSpriteIndex = GetRandomValue(0, maxAsteroidSprites - 1);

    int w = (int)asteroidSprites[asteroidSpriteIndex].textureRect.width;
    int h = (int)asteroidSprites[asteroidSpriteIndex].textureRect.height;

    asteroids[i].sprite = &asteroidSprites[asteroidSpriteIndex];
    asteroids[i].angle = (float)GetRandomValue(0, 360) - 180;
    asteroids[i].angleDelta = (float)GetRandomValue(-8, 8) / 64.0f;
    asteroids[i].position.x = (float)GetRandomValue(w, LEVEL_WIDTH - w);
    asteroids[i].position.y = (float)GetRandomValue(h, LEVEL_WIDTH - h);
    asteroids[i].delta = (Vector2) {
      .x = (float)GetRandomValue(-8, 8) / 64.0f,
      .y = (float)GetRandomValue(-8, 8) / 64.0f,
    };
  }
}

void initSoundEffects(void) {
  dashSoundEffect = LoadSound("resources/dash.wav");
  SetSoundVolume(dashSoundEffect, 0.3);

  playerShot = LoadSound("resources/shot01.wav");
  SetSoundVolume(playerShot, 0.5);

  beep = LoadSound("resources/beep.wav");
  hit = LoadSound("resources/hit.wav");
  SetSoundVolume(hit, 0.7);

  buttonFocusEffect = LoadSound("resources/hit.wav");
  SetSoundVolume(buttonFocusEffect, 0.1);

  borderActivation = LoadSound("resources/border.wav");
  SetSoundVolume(borderActivation, 0.3);

  bossMarineShotgunSound = LoadSound("resources/shot02.wav");
  bossMarineGunshotSound = LoadSound("resources/shot03.wav");

  playerDeathSound = LoadSound("resources/dead.wav");
}

void initShaders(void) {
  time = 0;

  {
#define NEBULAE_NOISE_DOWNSCALE_FACTOR 1

    Image n = GenImagePerlinNoise(background.x / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                  background.y / NEBULAE_NOISE_DOWNSCALE_FACTOR,
                                  0, 0, 4);

    nebulaNoise = LoadTextureFromImage(n);
    /* SetTextureFilter(nebulaNoise, TEXTURE_FILTER_BILINEAR); */
    UnloadImage(n);
  }

  {
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
    SetShaderValue(arenaBorderShader,
                   GetShaderLocation(arenaBorderShader, "resolution"),
                   &level,
                   SHADER_ATTRIB_VEC2);

  }

  {
    Vector4 dashResetGlowColor = ColorNormalize(ColorAlpha(SKYBLUE, 0.1f));

    dashResetShader = LoadShader(NULL, TextFormat("resources/dash-reset-glow-%d.frag", GLSL_VERSION));
    dashResetShaderAlpha = GetShaderLocation(dashResetShader, "alpha");
    SetShaderValue(dashResetShader,
                   GetShaderLocation(dashResetShader, "glowColor"),
                   &dashResetGlowColor,
                   SHADER_UNIFORM_VEC4);
  }

  {
    stars = LoadShader(NULL, TextFormat("resources/stars-%d.frag", GLSL_VERSION));
    starsTime = GetShaderLocation(stars, "time");
    SetShaderValue(stars,
                   GetShaderLocation(stars, "resolution"),
                   &background,
                   SHADER_UNIFORM_VEC2);
    float dom = 1.0f;
    SetShaderValue(stars,
                   GetShaderLocation(stars, "dom"),
                   &dom,
                   SHADER_UNIFORM_FLOAT);
  }

  {
    playerHealthOverlayShader = LoadShader(NULL, TextFormat("resources/health-overlay-%d.frag", GLSL_VERSION));

    /* Vector4 good = ColorNormalize((Color) {164, 36, 40, 255}); */
    Vector4 good = ColorNormalize((Color) {129, 73, 151, 255});
    SetShaderValue(playerHealthOverlayShader,
                   GetShaderLocation(playerHealthOverlayShader, "goodColor"),
                   &good,
                   SHADER_UNIFORM_VEC4);

    /* Vector4 bad = ColorNormalize((Color) {69, 69, 69, 255}); */
    Vector4 bad = ColorNormalize(BLACK);
    SetShaderValue(playerHealthOverlayShader,
                   GetShaderLocation(playerHealthOverlayShader, "badColor"),
                   &bad,
                   SHADER_UNIFORM_VEC4);

    playerHealthOverlayHealth = GetShaderLocation(playerHealthOverlayShader, "health");

    Image mask = LoadImage("resources/sprites.png");
    ImageCrop(&mask, playerHealthOverlayMaskRect);
    playerHealthMaskTexture = LoadTextureFromImage(mask);
    UnloadImage(mask);
  }

  {
    dashTrailShader = LoadShader(NULL, TextFormat("resources/dash-trail-%d.frag", GLSL_VERSION));
    Vector4 color = ColorNormalize(SKYBLUE);
    SetShaderValue(dashTrailShader,
                   GetShaderLocation(dashTrailShader, "trailColor"),
                   &color,
                   SHADER_UNIFORM_VEC4);

    dashTrailShaderAlpha = GetShaderLocation(dashTrailShader, "alpha");
  }
}

void initTextures(void) {
  sprites = LoadTexture("resources/sprites.png");

  target = LoadRenderTexture(LEVEL_WIDTH, LEVEL_HEIGHT);

  playerTexture = LoadRenderTexture(playerRect.width, playerRect.height);
  playerTexture1 = LoadRenderTexture(playerRect.width, playerRect.height);
}

void initThrusterTrails(void) {
  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    thrusterTrail[i].texture = LoadRenderTexture(playerRect.width, playerRect.height);
    thrusterTrail[i].alpha = 0.0f;
    thrusterTrail[i].origin = Vector2Zero();
    thrusterTrail[i].angle = 0.0f;
  }
}

void updateBackgroundAsteroid(void) {
  bigAssAsteroidPosition = Vector2Add(bigAssAsteroidPosition,
                                      bigAssAsteroidPositionDelta);
  bigAssAsteroidAngle += bigAssAsteroidAngleDelta;
}

void resetGame(void) {
  StopMusicStream(bossMarineMusic);

  initAsteroids();
  initPlayer();
  initBossMarine();
  initBackgroundAsteroid();
  initProjectiles();

  for (int i = 0; i < THRUSTER_TRAILS_MAX; i++) {
    thrusterTrail[i].alpha = 0.0f;
    thrusterTrail[i].origin = Vector2Zero();
    thrusterTrail[i].angle = 0.0f;
  }
}

static Vector2 previousMousePressLocation = {0};
static Vector2 previousMouseLocation = {0};
void updateAndRenderPauseScreen(void) {
  updateMouse();

  bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  if (pressed) {
    previousMousePressLocation = screenMouseLocation;
  }

  float mul = camera.zoom * 2;
  Font f = GetFontDefault();
  float fontSize = 25 * mul;
  float spacing = 2 * mul;

  float w = (float)GetScreenWidth();
  float h = (float)GetScreenHeight();

  float bw = w / 4.5;
  float bh = h / 8;
  float x = w / 2;
  float y = h / 2 - (bh);

  Rectangle buttonContinue = {x - (bw / 2), y - (bh / 2), bw, bh};
  Rectangle buttonQuit = {x - (bw / 2), y - (bh / 2) + (bh * 2), bw, bh};

  BeginDrawing(); {
    ClearBackground(BLACK);

    {
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

      if (gameState == GAME_BOSS) {
        renderBossHealthBar();
      }
    }

    DrawRectangle(0, 0, w, h, ColorAlpha(BLACK, 0.7));

    {
      if (!CheckCollisionPointRec(previousMouseLocation, buttonContinue) &&
          CheckCollisionPointRec(screenMouseLocation, buttonContinue)) {
        PlaySound(buttonFocusEffect);
      }


      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(previousMousePressLocation, buttonContinue) &&
          CheckCollisionPointRec(screenMouseLocation, buttonContinue)) {
        DrawRectangleRec(buttonContinue,
                         LIGHTGRAY);
      } else {
        DrawRectangleRec(buttonContinue,
                         WHITE);
      }

      if (CheckCollisionPointRec(screenMouseLocation, buttonContinue)) {
        DrawRectangleLinesEx(buttonContinue,
                             2 * mul,
                             LIGHTGRAY);
      }

      char *text = "CONTINUE";

      Vector2 size = MeasureTextEx(f, text, fontSize, spacing);

      Vector2 pos = (Vector2) {
        .x = (float)buttonContinue.x + buttonContinue.width / 2,
        .y = (float)buttonContinue.y + buttonContinue.height / 2,
      };

      DrawTextPro(f,
                  text,
                  pos,
                  Vector2Scale(size, 0.5f),
                  0,
                  fontSize,
                  spacing,
                  BLACK);
    }

    {

      if (!CheckCollisionPointRec(previousMouseLocation, buttonQuit) &&
          CheckCollisionPointRec(screenMouseLocation, buttonQuit)) {
        PlaySound(buttonFocusEffect);
      }

      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(previousMousePressLocation, buttonQuit) &&
          CheckCollisionPointRec(screenMouseLocation, buttonQuit)) {
        DrawRectangleRec(buttonQuit,
                         LIGHTGRAY);
      } else {
        DrawRectangleRec(buttonQuit,
                         WHITE);
      }

      if (CheckCollisionPointRec(screenMouseLocation, buttonQuit)) {
        DrawRectangleLinesEx(buttonQuit,
                             2 * mul,
                             LIGHTGRAY);
      }

      char *text = "QUIT";

      Vector2 size = MeasureTextEx(f, text, fontSize, spacing);

      Vector2 pos = (Vector2) {
        .x = (float)buttonQuit.x + buttonQuit.width / 2,
        .y = (float)buttonQuit.y + buttonQuit.height / 2,
      };

      DrawTextPro(f,
                  text,
                  pos,
                  Vector2Scale(size, 0.5f),
                  0,
                  fontSize,
                  spacing,
                  BLACK);
    }

    renderMouseCursor();
  } EndDrawing();

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    if (CheckCollisionPointRec(previousMousePressLocation, buttonContinue) &&
        CheckCollisionPointRec(screenMouseLocation, buttonContinue)) {
      PlaySound(beep);
      isGamePaused = false;
    } else if (CheckCollisionPointRec(previousMousePressLocation, buttonQuit) &&
               CheckCollisionPointRec(screenMouseLocation, buttonQuit)) {
      PlaySound(beep);
      isGamePaused = false;

      gameState = GAME_MAIN_MENU;
      resetGame();
    }
  }

  printf("%d\n", IsMouseButtonReleased(MOUSE_BUTTON_LEFT));

  previousMouseLocation = screenMouseLocation;
}

void updateAndRenderBossFight(void) {
  if (IsKeyPressed(KEY_ESCAPE)) {
    PlaySound(beep);
    isGamePaused = !isGamePaused;
  }

  if (isGamePaused) {
    PauseMusicStream(bossMarineMusic);
    updateAndRenderPauseScreen();
    return;
  }

  ResumeMusicStream(bossMarineMusic);

  updateCamera();

  if (!IsMusicStreamPlaying(bossMarineMusic)) {
    PlayMusicStream(bossMarineMusic);
  }

  UpdateMusicStream(bossMarineMusic);

  updateProjectiles();
  updateThrusterTrails();
  updatePlayerDashTrails();
  updateAsteroids();
  updateBackgroundAsteroid();

  updateMouse();
  updatePlayerPosition();
  updatePlayerCooldowns();

  updateBossMarine();

  tryDashing();
  tryFiringAShot();

  renderPhase1();
  renderFinal();
}

void renderGameTitle(void) {
  float mul = camera.zoom * 2;

  Font f = GetFontDefault();
  char *text = "STRIBUN";
  float fontSize = 50 * mul;
  float spacing = 4 * mul;
  Vector2 size = MeasureTextEx(f, text, fontSize, spacing);

  Vector2 pos = (Vector2) {
    .x = (float)GetScreenWidth() / 2,
    .y = (float)GetScreenHeight() / 6,
  };

  DrawTextPro(f,
              text,
              Vector2Add(pos, (Vector2) {spacing, spacing}),
              Vector2Scale(size, 0.5f),
              0,
              fontSize,
              spacing,
              BLACK);

  DrawTextPro(f,
              text,
              pos,
              Vector2Scale(size, 0.5f),
              0,
              fontSize,
              spacing,
              WHITE);
}

void renderPhase0(void) {
  BeginTextureMode(target); {
    renderBackground();
  } EndTextureMode();
}

static float floatingShipRotation = 0;
static float floatingShipRotationDelta = 360.0f / 30.0f;

void updateFloatingShip(void) {
  float halfWidth = (float)GetScreenWidth() / 2;

  float scale = 1;
  if (screenMouseLocation.x > halfWidth) {
    scale = (screenMouseLocation.x - halfWidth) / halfWidth;
  } else {
    scale = (halfWidth - screenMouseLocation.x) / halfWidth;
    scale *= -1;
  }

  floatingShipRotation += floatingShipRotationDelta * scale;
}

void renderFloatingShip(void) {
  float y = (float)GetScreenHeight() / 2;
  float x = (float)GetScreenWidth() / 4;

  float scale = camera.zoom * 30;

  float width = playerRect.width * scale;
  float height = playerRect.height * scale;

  DrawTexturePro(sprites,
                 playerRect,
                 (Rectangle) {x, y, width, height},
                 (Vector2) {width / 2, height / 2},
                 floatingShipRotation,
                 WHITE);
}

void renderMusicAuthor(void) {
  char* text = "Music: Drozerix - Stardust Jam";

  float mul = camera.zoom * 2;

  Font f = GetFontDefault();
  float fontSize = 10 * mul;
  float spacing = 1 * mul;
  Vector2 size = MeasureTextEx(f, text, fontSize, spacing);

  Vector2 pos = (Vector2) {
    .x = (float)10,
    .y = (float)GetScreenHeight() - size.y - 10,
  };

  float alpha = sin(time) * cos(time) + 0.5f;

  DrawTextPro(f,
              text,
              Vector2Add(pos, (Vector2) {spacing, spacing}),
              Vector2Zero(),
              0,
              fontSize,
              spacing,
              ColorAlpha(BLACK, alpha));

  DrawTextPro(f,
              text,
              pos,
              Vector2Zero(),
              0,
              fontSize,
              spacing,
              ColorAlpha(WHITE, alpha));
}

typedef enum {
  BUTTON_ACTION_START,
  BUTTON_ACTION_TOGGLE_CONTROLS,
#ifdef PLATFORM_DESKTOP
  BUTTON_ACTION_QUIT,
#endif
  BUTTON_ACTION_COUNT,
} ButtonAction;

typedef struct {
  ButtonAction action;
  Rectangle rect;
} Button;

static Button mainMenuButtons[BUTTON_ACTION_COUNT] = {0};
static Vector2 mousePressLocation = {0};
static Vector2 prevMouseLocation = {0};

void updateButtons(void) {
  float x = (float)GetScreenWidth() - (float)GetScreenWidth() / 4;
  float width = (float)GetScreenWidth() / 4.5;
  float height = (float)GetScreenHeight() / 8;
  float y = (float)GetScreenHeight() / 3 + (height);

#ifdef PLATFORM_WEB
  y += height;
#endif

  for (int i = 0; i < BUTTON_ACTION_COUNT; i++) {
    mainMenuButtons[i].action = i;
    mainMenuButtons[i].rect = (Rectangle) {
      x, y, width, height,
    };

    if (!CheckCollisionPointRec(prevMouseLocation, mainMenuButtons[i].rect) &&
        CheckCollisionPointRec(screenMouseLocation, mainMenuButtons[i].rect)) {
      PlaySound(buttonFocusEffect);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mousePressLocation, mainMenuButtons[i].rect) &&
        CheckCollisionPointRec(screenMouseLocation, mainMenuButtons[i].rect)) {
      switch (i) {
      case BUTTON_ACTION_START: {
        PlaySound(beep);
        gameState = GAME_TUTORIAL;
      } return;
      case BUTTON_ACTION_TOGGLE_CONTROLS: {
        PlaySound(beep);
        esdf = !esdf;
      } break;
#ifdef PLATFORM_DESKTOP
      case BUTTON_ACTION_QUIT: exit(0);
#endif
      default: break;
      }
    }

    y += height * 1.5;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    mousePressLocation = screenMouseLocation;
  }

  prevMouseLocation = screenMouseLocation;
}

void renderButtons(void) {
  float mul = camera.zoom * 2;
  Font f = GetFontDefault();
  float fontSize = 25 * mul;
  float spacing = 2 * mul;

  for (int i = 0; i < BUTTON_ACTION_COUNT; i++) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mousePressLocation, mainMenuButtons[i].rect) &&
        CheckCollisionPointRec(screenMouseLocation, mainMenuButtons[i].rect)) {
      DrawRectangleRec(mainMenuButtons[i].rect,
                       LIGHTGRAY);
    } else {
      DrawRectangleRec(mainMenuButtons[i].rect,
                       WHITE);
    }

    if (CheckCollisionPointRec(screenMouseLocation, mainMenuButtons[i].rect)) {
      DrawRectangleLinesEx(mainMenuButtons[i].rect,
                           2 * mul,
                           LIGHTGRAY);
    }

    char* text = "";

    switch (mainMenuButtons[i].action) {
    case BUTTON_ACTION_START: text = "START"; break;
    case BUTTON_ACTION_TOGGLE_CONTROLS: text = esdf ? "ESDF" : "WASD"; break;
#ifdef PLATFORM_DESKTOP
    case BUTTON_ACTION_QUIT: text = "QUIT"; break;
#endif
    default: continue;
    };

    Vector2 size = MeasureTextEx(f, text, fontSize, spacing);

    Vector2 pos = (Vector2) {
      .x = (float)mainMenuButtons[i].rect.x + mainMenuButtons[i].rect.width / 2,
      .y = (float)mainMenuButtons[i].rect.y + mainMenuButtons[i].rect.height / 2,
    };

    DrawTextPro(f,
                text,
                pos,
                Vector2Scale(size, 0.5f),
                0,
                fontSize,
                spacing,
                BLACK);
  }
}

void renderMainMenu(void) {
  renderPhase0();

  BeginDrawing(); {
    ClearBackground(BLACK);

    float width = (float)target.texture.width;
    float height = (float)target.texture.height;

    BeginMode2D(camera); {
      DrawTexturePro(target.texture,
                     (Rectangle) {0, 0, width, -height},
                     (Rectangle) {0, 0, width, height},
                     Vector2Zero(),
                     0.0f,
                     WHITE);
    }; EndMode2D();

    renderFloatingShip();
    renderGameTitle();
    renderMusicAuthor();
    renderButtons();
    renderMouseCursor();
  } EndDrawing();
}

void updateMainMenuMusic(void) {
  if (!IsMusicStreamPlaying(mainMenuMusic)) {
    PlayMusicStream(mainMenuMusic);
  }

  UpdateMusicStream(mainMenuMusic);
}

void updateAndRenderMainMenu(void) {
  updateCamera();
  updateMouse();
  updateFloatingShip();
  updateMainMenuMusic();
  updateButtons();

  renderMainMenu();
}

static float arenaLerp = 0;
static Vector2 arenaLerpLocation = {0};

static float bossInfoTimer = 0;

void updateAndRenderIntroduction(void) {
  UpdateMusicStream(bossMarineMusic);

  switch (introductionStage) {
  case BOSS_INTRODUCTION_BEGINNING: {
    cameraIntroductionTarget = (Vector2) {
      .x = (float)LEVEL_WIDTH / 2,
      .y = LEVEL_HEIGHT - (((float)LEVEL_HEIGHT / 6) / 2),
    };

    Vector2 playerDestination = {
      .x = (float)LEVEL_WIDTH / 2,
      .y = LEVEL_HEIGHT - ((float)LEVEL_HEIGHT / 6),
    };

    player.position = Vector2Lerp(player.position,
                                  playerDestination,
                                  0.05f);

    if (Vector2Distance(player.position, playerDestination) < 10.0f) {
      introductionStage = BOSS_INTRODUCTION_FOCUS;
      arenaLerpLocation = player.position;
      arenaLerp = 0;

      PlaySound(borderActivation);
          PlayMusicStream(bossMarineMusic);
    }
  } break;
  case BOSS_INTRODUCTION_FOCUS: {
    cameraIntroductionTarget = Vector2Lerp(cameraIntroductionTarget,
                                           bossMarine.position,
                                           0.1f);

    arenaLerp = Clamp(arenaLerp + GetFrameTime(), 0.0f, 1.0f);

    if (Vector2Distance(cameraIntroductionTarget, bossMarine.position) < 10.0f &&
        arenaLerp == 1.0f) {
      introductionStage = BOSS_INTRODUCTION_INFO;
      bossInfoTimer = 2.0f;

      bossInfoHeadPosition = (Vector2) {
          .x = -(GetScreenWidth() / 3),
          .y = (GetScreenHeight() / 2),
      };
      infoXBase = -roundf(GetScreenWidth() / 3);

      PlaySound(bossMarineShotgunSound);
    }
  } break;
  case BOSS_INTRODUCTION_INFO: {
    bossInfoTimer -= GetFrameTime();

    bossInfoHeadPosition =
      Vector2Lerp(bossInfoHeadPosition,
                  (Vector2) {
                    .x = (GetScreenWidth() / 3),
                    .y = (GetScreenHeight() / 2),
                  },
                  0.1f);

    infoXBase = Lerp(infoXBase, 0, 0.1f);

    if (bossInfoTimer <= 0.0f) {
      gameState = GAME_BOSS;
      isGamePaused = false;
      bossMarine.attackTimer = 0.5f;
      bossMarine.currentAttack = BOSS_MARINE_NOT_SHOOTING;
    }
  } break;
  }

  arenaTopLeft = Vector2Lerp(arenaLerpLocation, Vector2Zero(),
                             arenaLerp);
  arenaBottomRight = Vector2Lerp(arenaLerpLocation, level,
                                 arenaLerp);

  updateCamera();

  renderPhase1();
  renderFinal();
}

void updateAndRenderTutorial(void) {
  if (GetKeyPressed() != KEY_NULL ||
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
      seenTutorial) {
    PlaySound(beep);

    seenTutorial = true;
    gameState = GAME_BOSS_INTRODUCTION;
    introductionStage = BOSS_INTRODUCTION_BEGINNING;
    lookingDirection = Vector2Zero();
    return;
  }

  updateMouse();

  float mul = camera.zoom * 2;
  Font f = GetFontDefault();
  float fontSize = 25 * mul;
  float spacing = 2 * mul;

  char *text[] = {
    "W/A/S/D = MOVEMENT",
    "LEFT MOUSE BUTTON = SHOOT",
    "RIGHT MOUSE BUTTON = DASH",
  };

  if (esdf) {
    text[0] = "E/S/D/F = MOVEMENT";
  }

  Vector2 pos = (Vector2) {
    .x = (float)GetScreenWidth() / 2.0f,
    .y = (float)GetScreenHeight() / 3.0f,
  };

  updateCamera();
  BeginDrawing(); {
    ClearBackground(BLACK);

    for (size_t i = 0; i < sizeof(text) / sizeof(text[0]); i++) {
      Vector2 size = MeasureTextEx(f, text[i], fontSize, spacing);

      DrawTextPro(f, text[i], pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);
      pos.y += size.y;
    }

    char *text1 = "PRESS ANY KEY IF UNDERSTOOD";
    Vector2 size = MeasureTextEx(f, text1, fontSize, spacing);
    pos.y += size.y;

    DrawTextPro(f, text1, pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);

    renderMouseCursor();
  } EndDrawing();
}

void updateDeadBoss(void) {
  if (Vector2Distance(camera.target, bossMarine.position) > 5) {
    return;
  }

  const Vector2 headshotWeaponOffset = {
    .x = -45 * SPRITES_SCALE,
    .y = -19 * SPRITES_SCALE,
  };

  bossMarine.weaponOffset = Vector2Lerp(bossMarine.weaponOffset,
                                        headshotWeaponOffset,
                                        0.1f);

  bossMarine.weaponAngle = Lerp(bossMarine.weaponAngle, 0, 0.1f);

  if (Vector2Distance(bossMarine.weaponOffset, headshotWeaponOffset) < 0.5 &&
      fabsf(bossMarine.weaponAngle) < 0.1f) {
    PlaySound(bossMarineShotgunSound);
    ResumeMusicStream(bossMarineMusic);
    gameState = GAME_STATS;
  }
}

static float deadPlayerTime = 2.0f;

void updateDeadPlayer(void) {
  if (deadPlayerTime <= 0.0f) {
    ResumeMusicStream(bossMarineMusic);
    gameState = GAME_STATS;
  }
}

void updateAndRenderBossDead(void) {
  blackBackgroundAlpha = Lerp(blackBackgroundAlpha,
                              1.0f,
                              0.2f);
  blackBackgroundAlpha = Clamp(blackBackgroundAlpha,
                               0, 1);

  /* updateMouse(); */
  updateCamera();
  updateProjectiles();
  updateDeadBoss();

  renderPhase1();
  renderFinal();
}

void updateAndRenderPlayerDead(void) {
  blackBackgroundAlpha = Lerp(blackBackgroundAlpha,
                              1.0f,
                              0.2f);
  blackBackgroundAlpha = Clamp(blackBackgroundAlpha,
                               0, 1);

  deadPlayerTime -= GetFrameTime();

  /* updateMouse(); */
  updateCamera();
  updateProjectiles();
  updateDeadPlayer();

  renderPhase1();
  renderFinal();
}

void updateAndRenderStats(void) {
  if (GetKeyPressed() != KEY_NULL ||
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    PlaySound(beep);

    gameState = GAME_MAIN_MENU;
    resetGame();

    return;
  }

  UpdateMusicStream(bossMarineMusic);
  updateMouse();

  float mul = camera.zoom * 2;
  Font f = GetFontDefault();
  float fontSize = 25 * mul;
  float spacing = 2 * mul;

  BeginDrawing(); {
    ClearBackground(BLACK);

    const char *time_stat = TextFormat("TIME: %.2f", playerStats.time);
    Vector2 pos = {
      .x = ((float)GetScreenWidth() / 2.0f),
      .y = ((float)GetScreenHeight() / 2.0f),
    };

    Vector2 size = MeasureTextEx(f, time_stat, fontSize, spacing);

    pos.y -= size.y / 1.5;

    DrawTextPro(f, time_stat, pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);
    pos.y += size.y;

    const char *kills_stat = TextFormat("KILLS: %d", playerStats.kills);
    size = MeasureTextEx(f, kills_stat, fontSize, spacing);

    DrawTextPro(f, kills_stat, pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);

    const char *cont = "PRESS ANY KEY TO CONTINUE";
    pos.y += size.y;

    size = MeasureTextEx(f, cont, fontSize, spacing);
    DrawTextPro(f, cont, pos, Vector2Scale(size, 0.5f), 0, fontSize, spacing, WHITE);

    renderMouseCursor();
  } EndDrawing();
}

void UpdateDrawFrame(void) {
  switch (gameState) {
  case GAME_MAIN_MENU:
    updateAndRenderMainMenu();
    break;
  case GAME_TUTORIAL: {
    updateAndRenderTutorial();
  } break;
  case GAME_BOSS_INTRODUCTION:
    updateAndRenderIntroduction();
    break;
  case GAME_BOSS:
    updateAndRenderBossFight();
    playerStats.time += GetFrameTime();
    break;
  case GAME_BOSS_DEAD: {
    updateAndRenderBossDead();
  } break;
  case GAME_PLAYER_DEAD:
    updateAndRenderPlayerDead();
    break;
  case GAME_STATS:
    updateAndRenderStats();
    break;
  }

  time += GetFrameTime();
}

float randomFloat(void) {
  return (float)rand() / (float)RAND_MAX;
}

int main(void) {
  initRaylib();
  initMouse();
  initPlayer();
  initCamera();
  initProjectiles();
  initSoundEffects();
  initTextures();
  initShaders();
  initThrusterTrails();
  initAsteroids();
  initBackgroundAsteroid();
  initMusic();

  initBossMarine();

  seenTutorial = false;

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
