/// \cond
#ifndef CGLAB_H
#define CGLAB_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#include <Arduino.h>

#include "machineDependent.h"
#include "vector.h"

enum COLOR_ENUM {
  TRANSPARENT = -1,
  DARK1,
  DARK2,
  DARK3,
  LIGHT1,
  LIGHT2,
  LIGHT3,
  INVERT1,
  INVERT2,
  COLOR_COUNT,
  DEFAULT_COLOR = LIGHT1,
};

enum SOUND_ENUM {
  COIN = 0,
  LASER,
  EXPLOSION,
  POWER_UP,
  HIT,
  JUMP,
  SELECT,
  RANDOM,
  CLICK,
  SOUND_EFFECT_TYPE_COUNT,
  BGM = SOUND_EFFECT_TYPE_COUNT,
};

#define CHARACTER_WIDTH 6
#define CHARACTER_HEIGHT 6

#define TEXT_PATTERN_COUNT 94
#define MAX_CHARACTER_PATTERN_COUNT 26

#define HIT_BOX_INDEX_COLOR_BASE     0
#define HIT_BOX_INDEX_TEXT_BASE      (HIT_BOX_INDEX_COLOR_BASE + COLOR_COUNT)
#define HIT_BOX_INDEX_CHARACTER_BASE (HIT_BOX_INDEX_TEXT_BASE + TEXT_PATTERN_COUNT)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
/// \endcond

typedef struct {
  uint8_t x:4;
  uint8_t y:4;
  uint8_t w:4;
  uint8_t h:4;
} CharacterHitBox;

typedef struct {
  uint8_t grid[CHARACTER_WIDTH];
  CharacterHitBox hitBox;
} CharacterData;

typedef struct {
  uint16_t isColliding[8];
} Collision;

/// \cond
EXTERNC uint16_t ticks;
EXTERNC float score;
EXTERNC float difficulty;
EXTERNC int8_t color;
EXTERNC float thickness;
EXTERNC float barCenterPosRatio;
EXTERNC bool hasCollision;

EXTERNC bool colRect(Collision *cl, uint8_t color);
EXTERNC bool colText(Collision *cl, char text);
EXTERNC bool colCharacter(Collision *cl, char character);
EXTERNC Collision rect(float x, float y, float w, float h);
EXTERNC Collision box(float x, float y, float w, float h);
EXTERNC Collision line(float x1, float y1, float x2, float y2);
EXTERNC Collision bar(float x, float y, float length, float angle);
EXTERNC Collision arc(float centerX, float centerY, float radius,
                      float angleFrom, float angleTo);
EXTERNC Collision text(char *msg, float x, float y);
EXTERNC Collision constText(const char *msg, float x, float y);
EXTERNC Collision character(char character, float x, float y);
EXTERNC void play(uint8_t type);
EXTERNC void addScore(float value, float x, float y);
EXTERNC float rnd(float high, float low);
EXTERNC int16_t rndi(int16_t high, int16_t low);
EXTERNC void gameOver(void);
EXTERNC void particle(float x, float y, float count, float speed, float angle,
                      float angleWidth);
EXTERNC bool btn(uint8_t button);
EXTERNC bool btnp(uint8_t button);
EXTERNC bool btnr(uint8_t button);

EXTERNC float clamp(float v, float low, float high);
EXTERNC float wrap(float v, float low, float high);
EXTERNC void enableSound(void);
EXTERNC void disableSound(void);
EXTERNC void toggleSound(void);

EXTERNC void setupGame(const char *title, const char *description,
                       const CharacterData *characters, void (*update)(void),
                       const uint8_t **sounds);
EXTERNC void initGame(void);
EXTERNC void updateFrame(void);
/// \endcond

//! Iterate over an `array` with variable `index`
#define FOR_EACH(array, index) \
  for (uint8_t index = 0; index < sizeof(array) / sizeof(array[0]); index++)
//! Assign the `index` th item in the `array` to an `item` variable of `type`
#define ASSIGN_ARRAY_ITEM(array, index, type, item) type *item = &array[index]
//! Skip (continue) if the member `isAlive` of variable `item` is false.
#define SKIP_IS_NOT_ALIVE(item) \
  if (!item->isAlive) continue
//! Iterate `count` times with variable `index`
#define TIMES(count, index) for (uint8_t index = 0; index < count; index++)
//! Count the number of items in the array for which the `isAlive` member is
//! true and assigns it to a variable defined as the int variable `counter`
#define COUNT_IS_ALIVE(array, counter)                               \
  uint8_t counter = 0;                                               \
  do {                                                               \
    for (uint8_t i = 0; i < sizeof(array) / sizeof(array[0]); i++) { \
      if (array[i].isAlive) {                                        \
        counter++;                                                   \
      }                                                              \
    }                                                                \
  } while (0)
//! Set the `isAlive` member to false for all items in the `array`
#define INIT_UNALIVED_ARRAY(array)                                   \
  do {                                                               \
    for (uint8_t i = 0; i < sizeof(array) / sizeof(array[0]); i++) { \
      array[i].isAlive = false;                                      \
    }                                                                \
  } while (0)
//! Return 1 or -1 randomly
#define RNDPM() (rndi(0, 2) * 2 - 1)

#endif
