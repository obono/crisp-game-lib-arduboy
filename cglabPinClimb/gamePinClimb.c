#include <cglab.h>
#include "soundPattern.h"

PROGMEM static const char title[] = "PIN CLIMB";
PROGMEM static const char description[] = "[Hold] Stretch\0";

typedef struct {
  Vector pos;
  bool isAlive;
} Pin;
typedef struct {
  float angle;
  float length;
  Pin *pin;
} Cord;
static Cord cord;
#define PIN_CLIMB_MAX_PIN_COUNT 24
static Pin pins[PIN_CLIMB_MAX_PIN_COUNT];
static int pinIndex;
static Pin *lastPin;
static float nextPinDist;
static float cordLength = 7;

static void addPin(float x, float y) {
  ASSIGN_ARRAY_ITEM(pins, pinIndex, Pin, p);
  p->pos.x = x;
  p->pos.y = y;
  p->isAlive = true;
  lastPin = p;
  pinIndex = wrap(pinIndex + 1, 0, PIN_CLIMB_MAX_PIN_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(pins);
    pinIndex = 0;
    addPin(123, 32);
    nextPinDist = 5;
    cord.angle = 0;
    cord.length = cordLength;
    cord.pin = &pins[0];
    barCenterPosRatio = 0;
  }
  float scr = difficulty * 0.02;
  if (cord.pin->pos.x > 26) {
    scr += (cord.pin->pos.x - 26) * 0.1;
  }
  if (btnp(INPUT_A | INPUT_B)) {
    play(SELECT);
  }
  if (btn(INPUT_A | INPUT_B)) {
    cord.length += difficulty;
  } else {
    cord.length += (cordLength - cord.length) * 0.1;
  }
  cord.angle += difficulty * 0.05;
  bar(VEC_XY(cord.pin->pos), cord.length, cord.angle);
  Pin *nextPin = NULL;
  FOR_EACH(pins, i) {
    ASSIGN_ARRAY_ITEM(pins, i, Pin, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x -= scr;
    Collision cl = box(VEC_XY(p->pos), 3, 3);
    if (colRect(&cl, DEFAULT_COLOR) && p != cord.pin) {
      nextPin = p;
    }
    p->isAlive = p->pos.x >= -2;
  }
  if (nextPin != NULL) {
    play(POWER_UP);
    addScore(ceil(distanceTo(&cord.pin->pos, VEC_XY(nextPin->pos))),
             VEC_XY(nextPin->pos));
    cord.pin = nextPin;
    cord.length = cordLength;
  }
  nextPinDist -= scr;
  while (nextPinDist < 0) {
    float x = 130 + nextPinDist;
    float y;
    do {
      y = rnd(8, 56);
    } while (lastPin && distanceTo(&lastPin->pos, x, y) < cordLength * 2);
    addPin(x, y);
    nextPinDist += rnd(6, 19);
  }
  if (cord.pin->pos.x < 2) {
    play(EXPLOSION);
    gameOver();
  }
}

void setupGamePinClimb() {
  setupGame(title, description, NULL, soundPatterns, update);
}
