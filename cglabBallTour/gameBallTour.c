#include <cglab.h>
#include "soundPattern.h"

PROGMEM static const char title[] = "BALL TOUR";
PROGMEM static const char description[] = "[Hold] Move forward\0";

PROGMEM static const CharacterData characters[] = {
  {{ 0x0F, 0x39, 0x0F, 0x09, 0x3F, 0x0F }, { 0, 0, 6, 6 }},
  {{ 0x3E, 0x32, 0x1E, 0x12, 0x3E, 0x3E }, { 0, 1, 6, 5 }},
  {{ 0x00, 0x0E, 0x3B, 0x3D, 0x1F, 0x0E }, { 1, 0, 5, 6 }},
};

PROGMEM static const char spikeText[] = "*";

#define PLAYER_X 115
typedef struct {
  float yAngle;
  float vx;
  bool isAlive;
} Player;
static Player player;
typedef struct {
  int8_t x;
  float y;
  int8_t vy:7;
  int8_t isAlive:1;
} Spike;
#define BALL_TOUR_MAX_SPIKE_COUNT 24
static Spike spikes[BALL_TOUR_MAX_SPIKE_COUNT];
static uint8_t spikeIndex;
static int8_t nextSpikeDist;
typedef struct {
  int8_t x;
  int8_t y:7;
  int8_t isAlive:1;
} Ball;
#define BALL_TOUR_MAX_BALL_COUNT 12
static Ball balls[BALL_TOUR_MAX_BALL_COUNT];
static uint8_t ballIndex;
static int8_t nextBallDist;
static float multiplier;
static float scroll;

static void update() {
  if (!ticks) {
    player.yAngle = 0;
    player.vx = 0;
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    nextSpikeDist = 0;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    nextBallDist = 9;
    multiplier = 1;
    scroll = 0;
  }
  scroll += player.vx;
  int8_t scrollInt = (int8_t)scroll;
  scroll -= scrollInt;
  nextSpikeDist -= scrollInt;
  if (nextSpikeDist < 0) {
    ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, s);
    s->x = -9;
    s->y = rndi(12, 61);
    s->vy = rnd(0, 1) < 0.2 ? (rnd(1, difficulty) * 16) * RNDPM() : 0;
    s->isAlive = true;
    nextSpikeDist += rndi(12, 65);
    spikeIndex = wrap(spikeIndex + 1, 0, BALL_TOUR_MAX_SPIKE_COUNT);
  }
  FOR_EACH(spikes, i) {
    ASSIGN_ARRAY_ITEM(spikes, i, Spike, s);
    SKIP_IS_NOT_ALIVE(s);
    int16_t x = s->x + scrollInt;
    if (x <= INT8_MAX) {
      s->x = x;
      s->y += s->vy * (0.3 / 16.0);
      if (s->y < 12 || s->y > 60) {
        s->vy *= -1;
      }
      constText(spikeText, s->x, s->y);
    } else {
      s->isAlive = false;
    }
  }
  player.yAngle += difficulty * 0.05;
  if (player.yAngle >= M_PI * 2.0) {
    player.yAngle -= M_PI * 2.0;
  }
  float playerY = sinf(player.yAngle) * 20 + 36;
  if (btnp(INPUT_A | INPUT_B | INPUT_LEFT)) {
    play(HIT);
  }
  player.vx = (btn(INPUT_A | INPUT_B | INPUT_LEFT) ? 1 : 0.1) * difficulty;
  bool isRising = player.yAngle >= M_PI * 0.5 && player.yAngle < M_PI * 1.5;
  character('a' + (isRising && (ticks & 8)), PLAYER_X, playerY);
  Collision cl = character('c', PLAYER_X, playerY - 6);
  bool isGameOver = colText(&cl, '*');
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    int16_t x = b->x + scrollInt;
    if (x <= INT8_MAX) {
      b->x = x;
      cl = character('c', b->x, b->y);
      if (colCharacter(&cl, 'a') || colCharacter(&cl, 'b') || colCharacter(&cl, 'c')) {
        addScore((int)multiplier, PLAYER_X, playerY);
        multiplier += 10;
        play(SELECT);
        b->isAlive = false;
        continue;
      }
    } else {
      b->isAlive = false;
    }
  }
  nextBallDist -= scrollInt;
  if (nextBallDist < 0) {
    int8_t x = -3, y = rndi(20, 53);
    color = TRANSPARENT;
    cl = character('c', x, y);
    if (colText(&cl, '*')) {
      nextBallDist += 9;
    } else {
      ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, b);
      b->x = x;
      b->y = y;
      b->isAlive = true;
      nextBallDist += rndi(32, 83);
      ballIndex = wrap(ballIndex + 1, 0, BALL_TOUR_MAX_BALL_COUNT);
    }
  }
  multiplier = clamp(multiplier - 0.02 * difficulty, 1, 999);
  color = LIGHT1;
  char multiplierText[5];
  sprintf(multiplierText, "x%d", (uint16_t)multiplier);
  text(multiplierText, 3, 9);
  if (isGameOver) {
    play(EXPLOSION);
    gameOver();
  }
}

void setupGameBallTour() {
  setupGame(title, description, characters, soundPatterns, update);
}
