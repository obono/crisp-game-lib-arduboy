/// \cond
#include "cglab.h"
#include "machineDependent.h"
#include "particle.h"
#include "textPattern.h"
#include "vector.h"

enum STATE_ENUM {
  STATE_TITLE = 0,
  STATE_IN_GAME,
  STATE_GAME_OVER,
};
/// \endcond

//! A variable incremented by one every 1/60 of a second (readonly).
uint16_t ticks;
//! Game score.
float score;
//! A variable that is one at the beginning of the game, two after 1 minute, and
//! increasing by one every minute (readonly).
float difficulty;
//! Set the color for drawing rectangles, particles, texts, and characters.
//! Setting the color prior to `text()` and `character()` will recolor the
//! pixel art. Use `color = LIGHT1` to restore and use the original colors.
//! Colors are `DARK*`, `LIGHT*`, `INVERT*` and `TRANSPARENT`.
int8_t color;
//! Set the thickness for drawing `line()`, `bar()` and `arc()`.
float thickness;
//! A value from 0 to 1 that defines where the center coordinates are on the
//! `bar()`, default: 0.5.
float barCenterPosRatio;
//! If `hasCollision` is set to `true`, the drawn rectangle will have no
//! collision with other rectangles. Used to improve performance.
bool hasCollision;

static uint8_t state;

static const char *title;
static const char *description;
static const CharacterData *characters;
static void (*update)(void);
static const uint8_t **sounds;

// Collision
/// \cond
typedef struct {
  uint8_t index;
  int8_t  x;
  int8_t  y;
  uint8_t w;
  uint8_t h;
} HitBox;

#define MAX_HIT_BOX_COUNT 120
/// \endcond
static HitBox hitBoxes[MAX_HIT_BOX_COUNT];
static uint8_t hitBoxesIndex;

static void initCollision(Collision *collision) {
  memset(collision, 0, sizeof(Collision));
}

static bool testCollision(HitBox r1, HitBox r2) {
  int16_t ox = r2.x - r1.x;
  int16_t oy = r2.y - r1.y;
  return -r2.w < ox && ox < r1.w && -r2.h < oy && oy < r1.h;
}

static void checkHitBox(Collision *cl, HitBox hitBox) {
  for (uint8_t i = 0; i < hitBoxesIndex; i++) {
    HitBox hb = hitBoxes[i];
    if (testCollision(hb, hitBox)) {
      bitSet(cl->isColliding[(hb.index >> 4)], hb.index & 0xF);
    }
  }
}

/// \cond
#define VALID_HIT_LENGTH 64
/// \endcond
static bool isValidHitCoord(float x, float y) {
  return x >= -VALID_HIT_LENGTH && x < VIEW_SIZE_X + VALID_HIT_LENGTH &&
         y >= -VALID_HIT_LENGTH && y < VIEW_SIZE_Y + VALID_HIT_LENGTH;
}

static bool isCollidingSomething(Collision *cl, uint8_t index) {
  return cl->isColliding[(index >> 4)] & bit(index & 0xF);
}

//! Whether it's colliding with rects.
bool colRect(Collision *cl, uint8_t color) {
  if (color >= DARK1 && color < COLOR_COUNT) {
    uint8_t index = HIT_BOX_INDEX_COLOR_BASE + color;
    return isCollidingSomething(cl, index);
  }
  return false;
}

//! Whether it's colliding with texts.
bool colText(Collision *cl, char text) {
  if (text >= '!' && text <= '~') {
    uint8_t index = HIT_BOX_INDEX_TEXT_BASE + (text - '!');
    return isCollidingSomething(cl, index);
  }
  return false;
}

//! Whether it's colliding with characters.
bool colCharacter(Collision *cl, char character) {
  if (character >= 'a' && character <= 'z') {
    uint8_t index = HIT_BOX_INDEX_CHARACTER_BASE + (character - 'a');
    return isCollidingSomething(cl, index);
  }
  return false;
}

// Drawing
static uint8_t drawingHitBoxesIndex;

static void beginAddingRects(void) { drawingHitBoxesIndex = hitBoxesIndex; }

static void addRect(bool isAlignCenter, float x, float y, float w, float h,
                    Collision *hitCollision) {
  if (isAlignCenter) {
    x -= w / 2;
    y -= h / 2;
  }
  if (hasCollision && isValidHitCoord(x, y)) {
    HitBox hb;
    hb.index = HIT_BOX_INDEX_COLOR_BASE + color;
    hb.x = x;
    hb.y = y;
    hb.w = w;
    hb.h = h;
    checkHitBox(hitCollision, hb);
    if (color > TRANSPARENT && drawingHitBoxesIndex < MAX_HIT_BOX_COUNT) {
      hitBoxes[drawingHitBoxesIndex] = hb;
      drawingHitBoxesIndex++;
    }
  }
  if (color > TRANSPARENT && color < COLOR_COUNT) {
    md_drawRect(x, y, w, h, color);
  }
}

static void addHitBox(HitBox hb) {
  if (hitBoxesIndex < MAX_HIT_BOX_COUNT) {
    hitBoxes[hitBoxesIndex] = hb;
    hitBoxesIndex++;
  } else {
    // Too many hit boxes!
  }
}

static void endAddingRects(void) {
  hitBoxesIndex = drawingHitBoxesIndex;
}

//! Draw a rectangle. Returns information on objects that collided while drawing.
Collision rect(float x, float y, float w, float h) {
  Collision hitCollision;
  initCollision(&hitCollision);
  beginAddingRects();
  addRect(false, x, y, w, h, &hitCollision);
  endAddingRects();
  return hitCollision;
}

//! Draw a box. Returns information on objects that collided while drawing.
Collision box(float x, float y, float w, float h) {
  Collision hitCollision;
  initCollision(&hitCollision);
  beginAddingRects();
  addRect(true, x, y, w, h, &hitCollision);
  endAddingRects();
  return hitCollision;
}

static void drawLine(float x, float y, float ox, float oy,
                     Collision *hitCollision) {
  float lx = fabsf(ox);
  float ly = fabsf(oy);
  float t = thickness * 1.5f;
  t = clamp(t, 3, 10);
  uint8_t rn = ceilf(lx > ly ? lx / t : ly / t);
  rn = clamp(rn, 3, 49);
  ox /= (rn - 1);
  oy /= (rn - 1);
  for (uint8_t i = 0; i < rn; i++) {
    addRect(true, x, y, thickness, thickness, hitCollision);
    x += ox;
    y += oy;
  }
}

//! Draw a line. Returns information on objects that collided while drawing.
Collision line(float x1, float y1, float x2, float y2) {
  Collision hitCollision;
  initCollision(&hitCollision);
  beginAddingRects();
  drawLine(x1, y1, x2 - x1, y2 - y1, &hitCollision);
  endAddingRects();
  return hitCollision;
}

//! Draw a bar. Returns information on objects that collided while drawing.
Collision bar(float x, float y, float length, float angle) {
  Collision hitCollision;
  initCollision(&hitCollision);
  Vector l;
  rotate(vectorSet(&l, length, 0), angle);
  Vector p;
  vectorSet(&p, x - l.x * barCenterPosRatio, y - l.y * barCenterPosRatio);
  beginAddingRects();
  drawLine(p.x, p.y, l.x, l.y, &hitCollision);
  endAddingRects();
  return hitCollision;
}

//! Draw a arc. Returns information on objects that collided while drawing.
Collision arc(float centerX, float centerY, float radius, float angleFrom,
              float angleTo) {
  Collision hitCollision;
  initCollision(&hitCollision);
  beginAddingRects();
  float af, ao;
  if (angleFrom > angleTo) {
    af = angleTo;
    ao = angleFrom - angleTo;
  } else {
    af = angleFrom;
    ao = angleTo - angleFrom;
  }
  if (ao < 0.01f) {
    return hitCollision;
  }
  if (ao < 0) {
    ao = 0;
  } else if (ao > M_PI * 2) {
    ao = M_PI * 2;
  }
  uint8_t lc = ceilf(ao * sqrtf(radius * 0.125f));
  lc = clamp(lc, 1, 18);
  float ai = ao / lc;
  float a = af;
  float p1x = radius * cosf(a) + centerX;
  float p1y = radius * sinf(a) + centerY;
  float p2x, p2y;
  float ox, oy;
  for (uint8_t i = 0; i < lc; i++) {
    a += ai;
    p2x = radius * cosf(a) + centerX;
    p2y = radius * sinf(a) + centerY;
    ox = p2x - p1x;
    oy = p2y - p1y;
    drawLine(p1x, p1y, ox, oy, &hitCollision);
    p1x = p2x;
    p1y = p2y;
  }
  endAddingRects();
  return hitCollision;
}

// Text and character
static void drawCharacter(uint8_t index, float x, float y, bool _hasCollision,
                          bool isText, Collision *hitCollision) {
  if ((isText && (index < '!' || index > '~')) ||
      (!isText && (index < 'a' || index > 'z'))) {
    return;
  }
  if (color > TRANSPARENT && color < COLOR_COUNT &&
      x > -CHARACTER_WIDTH && x < VIEW_SIZE_X &&
      y > -CHARACTER_HEIGHT && y < VIEW_SIZE_Y) {
    const uint8_t *grid =
        isText ? textPatterns[index - '!'] : characters[index - 'a'].grid;
    md_drawCharacter(grid, x, y, color);
  }
  if (hasCollision && _hasCollision && isValidHitCoord(x, y)) {
    HitBox hb;
    if (isText) {
      hb.index = HIT_BOX_INDEX_TEXT_BASE + (index - '!');
      hb.x = x + 1;
      hb.y = y + 1;
      hb.w = CHARACTER_WIDTH - 1;
      hb.h = CHARACTER_HEIGHT - 1;
    } else {
      hb.index = HIT_BOX_INDEX_CHARACTER_BASE + (index - 'a');
      CharacterHitBox chb;
      *((uint16_t *)&chb) = pgm_read_word(&characters[index - 'a'].hitBox); 
      hb.x = x + chb.x;
      hb.y = y + chb.y;
      hb.w = chb.w;
      hb.h = chb.h;
    }
    checkHitBox(hitCollision, hb);
    if (color > TRANSPARENT) {
      addHitBox(hb);
    }
  }
}

static Collision drawConstCharacters(const char *msg, float x, float y,
                                     bool _hasCollision, bool isText) {
  Collision hitCollision;
  initCollision(&hitCollision);
  uint8_t ml = strlen_P(msg);
  x -= CHARACTER_WIDTH / 2;
  y -= CHARACTER_HEIGHT / 2;
  for (uint8_t i = 0; i < ml; i++) {
    drawCharacter(pgm_read_byte(&msg[i]), x, y, _hasCollision, isText,
                  &hitCollision);
    x += CHARACTER_WIDTH;
  }
  return hitCollision;
}

static Collision drawCharacters(char *msg, float x, float y, bool _hasCollision,
                                bool isText) {
  Collision hitCollision;
  initCollision(&hitCollision);
  uint8_t ml = strlen(msg);
  x -= CHARACTER_WIDTH / 2;
  y -= CHARACTER_HEIGHT / 2;
  for (uint8_t i = 0; i < ml; i++) {
    drawCharacter(msg[i], x, y, _hasCollision, isText, &hitCollision);
    x += CHARACTER_WIDTH;
  }
  return hitCollision;
}

//! Draw a text. Returns information on objects that collided while drawing.
Collision text(char *msg, float x, float y) {
  return drawCharacters(msg, x, y, true, true);
}

//! Draw a const text. Returns information on objects that collided while drawing.
Collision constText(const char *msg, float x, float y) {
  return drawConstCharacters(msg, x, y, true, true);
}

//! Draw a pixel art. Returns information on objects that collided while
//! drawing. You can define pixel arts (6x6 dots) of characters with
//! `characters` array.
Collision character(char character, float x, float y) {
  Collision hitCollision;
  initCollision(&hitCollision);
  x -= CHARACTER_WIDTH / 2;
  y -= CHARACTER_HEIGHT / 2;
  drawCharacter(character, x, y, true, false, &hitCollision);
  return hitCollision;
}

// Color
static void clearView(void) {
  md_clearView();
}

static uint8_t savedColor;

static void resetColor(void) {
  color = DEFAULT_COLOR;
}

static void saveCurrentColor(void) {
  savedColor = color;
  resetColor();
}

static void loadCurrentColor(void) {
  color = savedColor;
}

// Sound
static uint8_t soundType;

//! Play a sound effect. Type are `COIN`, `LASER`, `EXPLOSION`, `POWER_UP`,
//! `HIT`, `JUMP`, `SELECT`, `CLICK` and `RANDOM`.
void play(uint8_t type) {
  soundType = type;
}

//! Enable playing background music and sound effects.
void enableSound(void) {
  md_setSoundEnabled(true);
}

//! Disable playing background music and sound effects.
void disableSound(void) {
  md_setSoundEnabled(false);
}

//! Toggle sound playing flag.
void toggleSound(void) {
  md_setSoundEnabled(!md_getSoundEnabled());
}

PROGMEM static const uint8_t defaultSe[] = {
  0x90, 0x48, 0x00, 0x10, 0x90, 0x4B, 0x00, 0x11, 0x90,
  0x4E, 0x00, 0x11, 0x90, 0x51, 0x00, 0x10, 0x80, 0xF0,
};
PROGMEM static const uint8_t defaultBgm[] = {
  0x90, 0x3C, 0x00, 0x85, 0x80, 0x00, 0x85, 0x90, 0x3E, 0x00, 0x86, 0x80,
  0x00, 0x85, 0x90, 0x40, 0x00, 0x85, 0x80, 0x01, 0x90, 0x90, 0x41, 0x00,
  0x86, 0x80, 0x00, 0x85, 0x90, 0x40, 0x00, 0x85, 0x80, 0x00, 0x85, 0x90,
  0x3E, 0x00, 0x86, 0x80, 0x01, 0x90, 0x90, 0x39, 0x00, 0x85, 0x80, 0x00,
  0x85, 0x90, 0x3B, 0x00, 0x86, 0x80, 0x00, 0x85, 0x90, 0x3C, 0x00, 0x85,
  0x80, 0x01, 0x90, 0x90, 0x3E, 0x00, 0x86, 0x80, 0x00, 0x85, 0x90, 0x3C,
  0x00, 0x85, 0x80, 0x00, 0x85, 0x90, 0x3B, 0x00, 0x86, 0x80, 0x01, 0x90, 0xE0,
};

static void updateSound(void) {
  if ((ticks & 0x0F) == 0) {
    if (soundType >= 0 && soundType < SOUND_EFFECT_TYPE_COUNT) {
      const uint8_t *p = sounds ? pgm_read_ptr(&sounds[soundType]) : defaultSe;
      md_playSound(p, false);
    }
    soundType = -1;
  }
  if ((ticks & 0xFF) == 0) {
    if (state == STATE_IN_GAME) {
      const uint8_t *p = sounds ? pgm_read_ptr(&sounds[BGM]) : defaultBgm;
      md_playSound(p, true);
    } else {
      md_stopSound(true);
    }
  }
}

// Score
static uint16_t hiScore;

/// \cond
typedef struct {
  int8_t value;
  uint16_t halfX:6;
  uint16_t halfY:5;
  uint16_t ticks:5;
} ScoreBoard;

#define MAX_SCORE_BOARD_COUNT 4
/// \endcond
static ScoreBoard scoreBoards[MAX_SCORE_BOARD_COUNT];
static uint8_t scoreBoardsIndex;

static void initScore(void) { score = hiScore = 0; }

static void initScoreBoards(void) {
  for (uint8_t i = 0; i < MAX_SCORE_BOARD_COUNT; i++) {
    scoreBoards[i].ticks = 0;
  }
  scoreBoardsIndex = 0;
}

static void updateScoreBoards(void) {
  saveCurrentColor();
  for (uint8_t i = 0; i < MAX_SCORE_BOARD_COUNT; i++) {
    ScoreBoard *sb = &scoreBoards[i];
    if (sb->ticks > 0) {
      char sc[5];
      bool isPositive = sb->value >= 0;
      uint8_t ll = sprintf(sc + isPositive, "%d", sb->value);
      if (isPositive) {
        sc[0] = '+';
        ll++;
      }
      float x = sb->halfX * 2 - (ll - 1) * CHARACTER_WIDTH / 2;
      float y = sb->halfY * 2 + sb->ticks * sb->ticks / 45.0f;
      drawCharacters(sc, x, y, false, true);
      sb->ticks--;
    }
  }
  loadCurrentColor();
}

//! Add score points and draw additional score on the screen. You can also add
//! score points simply by adding the `score` variable.
void addScore(float value, float x, float y) {
  if (value < 0 && score < -value) {
    score = 0;
  } else {
    score += value;
  }
  if (value >= INT8_MAX || value < INT8_MIN) {
    return;
  }
  ScoreBoard *sb = &scoreBoards[scoreBoardsIndex];
  sb->halfX = (uint8_t)clamp(x, 0, 127) / 2;
  y -= 20;
  sb->halfY = (uint8_t)clamp(y, 0, 63) / 2;
  sb->value = value;
  sb->ticks = 30;
  scoreBoardsIndex++;
  if (scoreBoardsIndex >= MAX_SCORE_BOARD_COUNT) {
    scoreBoardsIndex = 0;
  }
}

static void drawScore(void) {
  saveCurrentColor();
  color = DEFAULT_COLOR;
  char sc[9];
  sprintf(sc, "%d", (uint16_t)score);
  drawCharacters(sc, 3, 3, false, true);
  sc[0] = 'H';
  sc[1] = 'I';
  sc[2] = ' ';
  uint8_t ll = sprintf(sc + 3, "%d", hiScore) + 3;
  drawCharacters(sc, VIEW_SIZE_X - ll * 6 + 2, 3, false, true);
  loadCurrentColor();
}

//! Add particles.
void particle(float x, float y, float count, float speed, float angle,
              float angleWidth) {
  addParticle(x, y, count, speed, angle, angleWidth);
}

// Input
static uint8_t input = ~0;
static uint8_t lastInput;

static void updateInput(void) {
  lastInput = input;
  input = md_getInputState();
}

//! Whether the button is being pressed. Button are `INPUT_*`.
bool btn(uint8_t button) {
  return input & button;
}

//! Whether the button is just pressed now. Button are `INPUT_*`.
bool btnp(uint8_t button) {
  return input & (input ^ lastInput) & button;
}

//! Whether the button is just released now. Button are `INPUT_*`.
bool btnr(uint8_t button) {
  return ~input & (input ^ lastInput) & button;
}

// Utilities
//! Get a random float value of [low, high).
float rnd(float low, float high) {
  return (float)(rand() & 0x7FFF) / (float)0x7FFF * (high - low) + low;
}

//! Get a random int value of [low, high - 1].
int16_t rndi(int16_t low, int16_t high) { return rand() % (high - low) + low; }

//! Clamp a value to [low, high].
float clamp(float v, float low, float high) {
  return fmaxf(low, fminf(v, high));
}

//! Wrap a value to [low, high).
float wrap(float v, float low, float high) {
  float w = high - low;
  float o = v - low;
  if (o >= 0) {
    return fmodf(o, w) + low;
  } else {
    float wv = w + fmodf(o, w) + low;
    if (wv >= high) {
      wv -= w;
    }
    return wv;
  }
}

// In game
static void resetDrawState(void) {
  resetColor();
  thickness = 3;
  barCenterPosRatio = 0.5f;
  hasCollision = true;
}

static void initInGame(void) {
  state = STATE_IN_GAME;
  if (score > hiScore) {
    hiScore = score;
  }
  score = 0;
  initScoreBoards();
  initParticle();
  resetDrawState();
  soundType = -1;
  ticks = -1;
}

static void updateInGame(void) {
  clearView();
  update();
  updateParticles();
  updateScoreBoards();
}

// Title
/// \cond
#define MAX_DESCRIPTION_LINE_COUNT 5
#define MAX_DESCRIPTION_STRLEN 21
/// \endcond
static uint8_t descriptionLineCount;
static int8_t descriptionX;

static void parseDescription(void) {
  descriptionLineCount = 0;
  uint8_t dl = 0, ll;
  const char *line = description;
  while ((ll = strlen_P(line)) > 0) {
    uint8_t lln = ll;
    if (lln > MAX_DESCRIPTION_STRLEN) {
      lln = MAX_DESCRIPTION_STRLEN;
    }
    if (lln > dl) {
      dl = lln;
    }
    descriptionLineCount++;
    if (descriptionLineCount >= MAX_DESCRIPTION_LINE_COUNT) {
      break;
    }
    line += ll + 1;
  };
  descriptionX = (VIEW_SIZE_X - (dl - 1) * CHARACTER_WIDTH) / 2;
}

static void initTitle(void) {
  state = STATE_TITLE;
  ticks = -1;
  resetDrawState();
}

PROGMEM static const char soundImagePattern[] = {
  0x35, 0x35, 0x09, 0x32, 0x04, 0x38,
};

static void updateTitle(void) {
  if (btnp(INPUT_A | INPUT_B)) {
    md_saveSoundEnabled();
    srand(rand() ^ ticks);
    initInGame();
    return;
  }
  if (btnp(INPUT_DOWN)) {
    toggleSound();
    if (md_getSoundEnabled()) {
      play(CLICK);
    }
  }
  saveCurrentColor();
  if (!ticks) {
    clearView();
    drawConstCharacters(title,
                        (VIEW_SIZE_X - (strlen_P(title) - 1) * CHARACTER_WIDTH) / 2,
                        VIEW_SIZE_Y * 0.25f, false, true);
  } else if (ticks == 30) {
    const char *line = description;
    for (uint8_t i = 0; i < descriptionLineCount; i++) {
      drawConstCharacters(line, descriptionX,
                          VIEW_SIZE_Y * 0.55f + i * CHARACTER_HEIGHT, false,
                          true);
      line += strlen_P(line) + 1;
    }
  }
  color = md_getSoundEnabled() ? LIGHT1 : DARK1;
  md_drawCharacter(soundImagePattern, 0, VIEW_SIZE_Y - CHARACTER_HEIGHT, color);
  loadCurrentColor();
}

// Game over
PROGMEM static const char gameOverText[] = "GAME OVER";

static void drawGameOver(void) {
  float x = (VIEW_SIZE_X - strlen_P(gameOverText) * CHARACTER_WIDTH) / 2;
  float y = VIEW_SIZE_Y * 0.5f;
  color = DARK1;
  for (int8_t i = 1; i <= 7; i += 2) {
    drawConstCharacters(gameOverText, x + (i % 3) - 1, y + (i / 3) - 1,
                        false, true);
  }
  color = LIGHT1;
  drawConstCharacters(gameOverText, x, y, false, true);
}

static void initGameOver(void) {
  state = STATE_GAME_OVER;
  saveCurrentColor();
  drawGameOver();
  loadCurrentColor();
  ticks = -1;
}

static void updateGameOver(void) {
  if (ticks >= 60 && btnp(INPUT_A | INPUT_B)) {
    initInGame();
  } else if (ticks >= 300) {
    initTitle();
  }
}

//! Transit to the game-over state.
void gameOver(void) {
  initGameOver();
}

// Initialize
//! Setup game.
void setupGame(const char *_title,
               const char *_description,
               const CharacterData *_characters,
               void (*_update)(void),
               const uint8_t **_sounds) {
  title = _title;
  description = _description;
  characters = _characters;
  update = _update;
  sounds = _sounds;
}

//! Initialize game.
void initGame(void) {
  md_initMachine();
  initScore();
  initParticle();
  parseDescription();
  resetDrawState();
  initTitle();
  soundType = -1;
  ticks = 0;
}

//! Update game frames if it's every 1/60 second timing.
bool updateGame(void) {
  if (!md_nextFrame()) {
    return false;
  }
  hitBoxesIndex = 0;
  difficulty = (float)ticks / 60 / FPS + 1;
  updateInput();
  if (state == STATE_TITLE) {
    updateTitle();
  } else if (state == STATE_IN_GAME) {
    updateInGame();
  } else if (state == STATE_GAME_OVER) {
    updateGameOver();
  }
  updateSound();
  drawScore();
  ticks++;
  md_refresh();
  return true;
}
