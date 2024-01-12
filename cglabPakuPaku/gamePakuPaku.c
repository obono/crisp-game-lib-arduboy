#include <cglab.h>
#include "soundPattern.h"

PROGMEM static const char title[] = "PAKU PAKU";
PROGMEM static const char description[] = "[Button] Turn\0";

PROGMEM static const CharacterData characters[] = {
  {{ 0x0C, 0x1E, 0x3F, 0x33, 0x21, 0x21 }, { 0, 0, 6, 6 }}, // 'a' Player Right 1
  {{ 0x21, 0x21, 0x33, 0x3F, 0x1E, 0x0C }, { 0, 0, 6, 6 }}, // 'b' Player Left 1
  {{ 0x0C, 0x1E, 0x3F, 0x33, 0x33, 0x12 }, { 0, 0, 6, 6 }}, // 'c' Player Right 2
  {{ 0x12, 0x33, 0x33, 0x3F, 0x1E, 0x0C }, { 0, 0, 6, 6 }}, // 'd' Player Left 2
  {{ 0x0C, 0x1E, 0x3F, 0x3F, 0x1E, 0x0C }, { 0, 0, 6, 6 }}, // 'e' Player Right 3
  {{ 0x0C, 0x1E, 0x3F, 0x3F, 0x1E, 0x0C }, { 0, 0, 6, 6 }}, // 'f' Player Left 3
  {{ 0x30, 0x1E, 0x3D, 0x1F, 0x2D, 0x02 }, { 0, 0, 6, 6 }}, // 'g' Enemy Right 1
  {{ 0x02, 0x2D, 0x1F, 0x3D, 0x1E, 0x30 }, { 0, 0, 6, 6 }}, // 'h' Enemy Left 1
  {{ 0x00, 0x3E, 0x1D, 0x3F, 0x1D, 0x02 }, { 1, 0, 5, 6 }}, // 'i' Enemy Right 2
  {{ 0x02, 0x1D, 0x3F, 0x1D, 0x3E, 0x00 }, { 0, 0, 5, 6 }}, // 'j' Enemy Left 2
  {{ 0x00, 0x00, 0x0C, 0x0C, 0x00, 0x00 }, { 2, 2, 2, 2 }}, // 'k' Dot
  {{ 0x00, 0x0C, 0x1E, 0x1E, 0x0C, 0x00 }, { 1, 1, 4, 4 }}, // 'l' Power
  {{ 0x00, 0x00, 0x02, 0x00, 0x02, 0x00 }, { 2, 1, 3, 1 }}, // 'm' Eyes Right
  {{ 0x00, 0x02, 0x00, 0x02, 0x00, 0x00 }, { 1, 1, 3, 1 }}, // 'n' Eyes Left
};

typedef struct {
  float x;
  float vx;
} Player;

typedef struct {
  float x;
  int8_t eyeVx;
} Enemy;

typedef struct {
  float x;
  bool isPower;
  bool isActive;
} Dot;

#define DOTS_COUNT_MAX  21
#define OBJECT_Y        36

static Player player;
static Enemy enemy;
static Dot dots[DOTS_COUNT_MAX];
static uint8_t dots_count;
static float powerTicks;
static uint8_t animTicks;
static uint16_t multiplier;
static uint8_t soundTicks;

static void addDots() {
  uint8_t pi = (player.x > 64) ? rndi(1, 8) : rndi(13, 20);
  for (uint8_t i = 0; i < DOTS_COUNT_MAX; i++) {
    dots[i].x = i * 6 + 4;
    dots[i].isPower = i == pi;
    dots[i].isActive = true;
  }
  dots_count = DOTS_COUNT_MAX;
  multiplier++;
}

static bool colPlayer(Collision *cl) {
  return colCharacter(cl, 'a') || colCharacter(cl, 'b') ||
         colCharacter(cl, 'c') || colCharacter(cl, 'd') ||
         colCharacter(cl, 'e') || colCharacter(cl, 'f');
}

static void update() {
  if (!ticks) {
    player.x = 51;
    player.vx = 1;
    enemy.x = 128;
    enemy.eyeVx = 0;
    multiplier = 0;
    addDots();
    powerTicks = 0;
    animTicks = 0;
    soundTicks = 0;
  }
  if (soundTicks > 0) {
    soundTicks--;
  }
  animTicks += (uint8_t)(difficulty * 8);
  color = LIGHT1;
  char multiplierText[5];
  sprintf(multiplierText, "x%d", (uint16_t)multiplier);
  text(multiplierText, 3, 9);
  if (player.vx > 0 && btn(INPUT_LEFT)) {
    player.vx = -1;
  } else if (player.vx < 0 && btn(INPUT_RIGHT)) {
    player.vx = 1;
  } else if (btnp(INPUT_A | INPUT_B)) {
    player.vx *= -1;
  }
  player.x += player.vx * 0.5 * difficulty;
  if (player.x < -3) {
    player.x = 131;
  } else if (player.x > 131) {
    player.x = -3;
  }
  rect(0, OBJECT_Y - 7, 128, 1);
  rect(0, OBJECT_Y - 5, 128, 1);
  rect(0, OBJECT_Y + 4, 128, 1);
  rect(0, OBJECT_Y + 6, 128, 1);
  color = LIGHT2;
  uint8_t ai = (animTicks >> 6) % 4;
  character(
    'a' + (player.vx < 0) + ((ai == 3) ? 2 : ai * 2),
    player.x,
    OBJECT_Y
  );
  for (uint8_t i = 0; i < DOTS_COUNT_MAX; i++) {
    Dot *d = &dots[i];
    if (!d->isActive) continue;
    color = (d->isPower && (animTicks >> 6) % 2 == 0) ? TRANSPARENT : LIGHT2;
    Collision clD = character(d->isPower ? 'l' : 'k', d->x, OBJECT_Y);
    if (colPlayer(&clD)) {
      if (d->isPower) {
        play(JUMP);
        soundTicks = 16;
        if (enemy.eyeVx == 0) {
          powerTicks = 154;
        }
      } else {
        if (!soundTicks) {
          play(HIT);
        }
      }
      score += multiplier;
      d->isActive = false;
      dots_count--;
    }
  }
  float evx =
    (enemy.eyeVx != 0)
      ? enemy.eyeVx
      : ((player.x > enemy.x) ? 1 : -1) * ((powerTicks > 0) ? -1 : 1);
  enemy.x = clamp(
    enemy.x +  
      evx *
        ((powerTicks > 0) ? 0.25 : (enemy.eyeVx != 0) ? 0.75 : 0.55) *
        difficulty,
    0,
    128
  );
  if ((enemy.eyeVx < 0 && enemy.x < 1) || (enemy.eyeVx > 0 && enemy.x > 127)) {
    enemy.eyeVx = 0;
  }
  bool isBlink = ((uint8_t)powerTicks % 10 < 5 || powerTicks >= 40);
  color = (powerTicks > 0 && isBlink && (ticks & 1)) ? DARK1 : LIGHT2;
  Collision cl = character(
    ((enemy.eyeVx != 0) ? 'm' : ('g' + (animTicks >> 6) % 2 * 2)) + (evx < 0),
    enemy.x,
    OBJECT_Y
  );
  if (enemy.eyeVx == 0 && colPlayer(&cl)) {
    if (powerTicks > 0) {
      play(POWER_UP);
      soundTicks = 16;
      addScore(10 * multiplier, enemy.x, OBJECT_Y);
      enemy.eyeVx = (player.x > 64) ? -1 : 1;
      powerTicks = 0;
      multiplier++;
    } else {
      play(EXPLOSION);
      gameOver();
    }
  }
  powerTicks -= difficulty;
  if (dots_count == 0) {
    play(COIN);
    soundTicks = 16;
    addDots();
  }
}

void setupGamePakuPaku(void) {
  setupGame(title, description, characters, soundPatterns, update);
}
