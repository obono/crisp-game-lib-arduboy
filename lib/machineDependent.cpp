#include <Arduboy2.h>

#include "machineDependent.h"

static void initAudio(void);
static void closeAudio(void);
static void playSound(uint8_t chan, const uint8_t *sound);
static void stopSound(uint8_t chan);
static void stopAllSounds(void);
static void forwardSound(uint8_t chan);
static uint16_t playNote(uint8_t chan, uint8_t note);
static uint16_t stopNote(uint8_t chan);
static void setupSoundTimer(uint8_t chan, uint16_t frequency);

/*---------------------------------------------------------------------------*/

static Arduboy2Base arduboy;

void md_initMachine(void) {
  arduboy.beginDoFirst();
  arduboy.setFrameRate(FPS);
  initAudio();
}

bool md_nextFrame(void) {
  return arduboy.nextFrame();
}

void md_refresh(void) {
  arduboy.display();
}

void md_clearView(void) {
  arduboy.clear();
}

PROGMEM static const uint8_t colorTable[] = {
  BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, INVERT, INVERT
};
#define getColor(color) (pgm_read_byte(&colorTable[color]))

void md_drawPixel(float _x, float _y, int8_t color) {
  /*  Check parameters  */
  int16_t x = (int16_t)_x, y = (int16_t)_y;
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

  /*  Draw a pixel  */
  uint8_t *p = arduboy.getBuffer() + x + (y / 8) * WIDTH;
  uint8_t d = bit(y & 7), c = getColor(color);
  if (c != INVERT) *p |= d;
  if (c != WHITE) *p ^= d;
}

void md_drawRect(float _x, float _y, float _w, float _h, int8_t color) {
  /*  Check parameters  */
  int16_t x = (int16_t)_x, y = (int16_t)_y, w = (int16_t)_w, h = (int16_t)_h;
  if (x < 0) {
    if (w <= -x) return;
    w += x;
    x = 0;
  }
  if (y < 0) {
    if (h <= -y) return;
    h += y;
    y = 0;
  }
  if (w <= 0 || x >= WIDTH || h <= 0 || y >= HEIGHT) return;
  if (x + w > WIDTH) w = WIDTH - x;
  if (y + h > HEIGHT) h = HEIGHT - y;

  /*  Draw a filled rectangle  */
  uint8_t *p = arduboy.getBuffer() + x + (y / 8) * WIDTH;
  uint8_t yOdd = y & 7, d = 0xFF << yOdd, c = getColor(color);
  for (h += yOdd; h > 0; h -= 8, p += WIDTH - w) {
    if (h < 8) d &= 0xFF >> (8 - h);
    if (c == WHITE) {
      for (uint8_t i = w; i > 0; i--, *p++ |= d) { ; }
    } else if (c == BLACK) {
      for (uint8_t i = w, invD = ~d; i > 0; i--, *p++ &= invD) { ; }
    } else {
      for (uint8_t i = w; i > 0; i--, *p++ ^= d) { ; }
    }
    d = 0xFF;
  }
}

void md_drawCharacter(const uint8_t grid[CHARACTER_WIDTH], float x, float y,
                      int8_t color) {
  arduboy.drawBitmap((int16_t)x, (int16_t)y, grid,
                     CHARACTER_WIDTH, CHARACTER_HEIGHT, getColor(color));
}

uint8_t md_getInputState(void) {
  return arduboy.buttonsState();
}

bool md_getSoundEnabled(void) {
  return arduboy.audio.enabled();
}

void md_setSoundEnabled(bool isEnabled) {
  isEnabled ? arduboy.audio.on() : arduboy.audio.off();
}

void md_saveSoundEnabled(void) {
  arduboy.audio.saveOnOff();
}

void md_playSound(const uint8_t *sound, bool isBgm) {
  playSound(isBgm ? 1 : 0, sound);
}

void md_stopSound(bool isBgm) {
  stopSound(isBgm ? 1 : 0);
}

/*---------------------------------------------------------------------------*/

#define MAX_CHANNEL 2

#define TIMER3_PIN_PORT SPEAKER_1_PORT
#define TIMER3_PIN_BIT  SPEAKER_1_BIT
#define TIMER1_PIN_PORT SPEAKER_2_PORT
#define TIMER1_PIN_BIT  SPEAKER_2_BIT

#define setHighTimerPin(t) \
  (bitSet(TIMER ## t ## _PIN_PORT, TIMER ## t ## _PIN_BIT))
#define setLowTimerPin(t) \
  (bitClear(TIMER ## t ## _PIN_PORT, TIMER ## t ## _PIN_BIT))
#define toggleTimerPin(t) \
  (bitToggle(TIMER ## t ## _PIN_PORT, TIMER ## t ## _PIN_BIT))

#define initTimer(t) do { \
          TCCR ## t ## A = 0; \
          TCCR ## t ## B = _BV(WGM ## t ## 2) | _BV(CS ## t ## 0); \
        } while (false)

#define startTimer(t, ocr, prescalerBits) do { \
          TCCR ## t ## B = (TCCR ## t ## B & 0b11111000) | (prescalerBits); \
          OCR ## t ## A = (ocr); \
          TCNT ## t = 0; \
          bitSet(TIMSK ## t, OCIE ## t ## A); \
        } while (false)

#define stopTimer(t) do { \
            bitClear(TIMSK ## t, OCIE ## t ## A); \
            setLowTimerPin(t); \
        } while (false)

static void initAudio(void) {
  power_timer3_enable();
  power_timer1_enable();
  initTimer(3);
  initTimer(1);
  stopAllSounds();
}

static void closeAudio(void) {
  stopAllSounds();
  power_timer3_disable();
  power_timer1_disable();
}

static volatile const uint8_t *soundData[MAX_CHANNEL];
static volatile uint32_t pinToggleCount[MAX_CHANNEL];
static volatile uint8_t playingFlag;

static void playSound(uint8_t chan, const uint8_t *sound) {
  if (chan >= MAX_CHANNEL) return;
  stopSound(chan);
  soundData[chan] = sound;
  forwardSound(chan);
}

static void stopSound(uint8_t chan) {
  if (chan >= MAX_CHANNEL) return;
  switch (chan) {
  case 0:
    stopTimer(3);
    break;
  case 1:
    stopTimer(1);
    break;
  }
  bitClear(playingFlag, chan);
  soundData[chan] = NULL;
}

static void stopAllSounds(void) {
  for (uint8_t chan = 0; chan < MAX_CHANNEL; chan++) {
    stopSound(chan);
  }
}

#define COMMAND_PLAY_NOTE 0x90 // play a note, note is next byte
#define COMMAND_STOP_NOTE 0x80 // stop a note
#define COMMAND_STOP      0xF0 // stop playing
#define COMMAND_RESTART   0xE0 // stop playing (not restart)

#define FREQUENCY_IDLE    1000
#define NOTE_MAX          127
#define NOTE_MIDDLE       sizeof(midiNoteFreqByte)

PROGMEM static const uint8_t midiNoteFreqByte[] = {
  16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46,
  49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247
};
PROGMEM static const uint16_t midiNoteFreqWord[] = {
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587,
  622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319,
  1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794,
  2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920,
  6272, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840,
  12544, 13290, 14080, 14917, 15804, 16744, 17740, 18795, 19912, 21096, 22351,
  23680, 25088
};

static void forwardSound(uint8_t chan) {
  const volatile uint8_t **pData = &soundData[chan];
  volatile uint32_t *pCount = &pinToggleCount[chan];
  while (*pData) {
    uint8_t command = pgm_read_byte((*pData)++);
    uint16_t frequency;
    if (command == COMMAND_PLAY_NOTE) {
      frequency = playNote(chan, pgm_read_byte((*pData)++));
    } else if (command == COMMAND_STOP_NOTE) {
      frequency = stopNote(chan);
    } else if (command < 0x80) { // wait count in msec.
      uint16_t duration = (uint16_t)command << 8 | pgm_read_byte((*pData)++);
      *pCount = ((uint32_t)duration * frequency + 500) / 1000;
      if (*pCount == 0) *pCount = 1;
      break;
    } else if (command == COMMAND_STOP || command == COMMAND_RESTART) {
      stopSound(chan);
      break;
    }
  }
}

static uint16_t playNote(uint8_t chan, uint8_t note) {
  if (note > NOTE_MAX) return stopNote(chan);
  uint16_t frequency = (note < NOTE_MIDDLE) ?
                       pgm_read_byte(&midiNoteFreqByte[note]) :
                       pgm_read_word(&midiNoteFreqWord[note - NOTE_MIDDLE]);
  bitSet(playingFlag, chan);
  setupSoundTimer(chan, frequency);
  return frequency;
}

static uint16_t stopNote(uint8_t chan) {
  bitClear(playingFlag, chan);
  setupSoundTimer(chan, FREQUENCY_IDLE);
  return FREQUENCY_IDLE;
}

static void setupSoundTimer(uint8_t chan, uint16_t frequency) {
  uint32_t ocr = F_CPU / frequency - 1;
  uint8_t prescalerBits = 0b001;
  if (ocr > 0xFFFF) {
    ocr = F_CPU / 64 / frequency - 1;
    prescalerBits = 0b011;
  }
  switch (chan) {
  case 0:
    startTimer(3, ocr, prescalerBits);
    break;
  case 1:
    startTimer(1, ocr, prescalerBits);
    break;
  }
}

ISR(TIMER3_COMPA_vect) {
  if (bitRead(playingFlag, 0)) toggleTimerPin(3);
  if (pinToggleCount[0] > 0 && --pinToggleCount[0] == 0) forwardSound(0);
}

ISR(TIMER1_COMPA_vect) {
  if (bitRead(playingFlag, 1)) toggleTimerPin(1);
  if (pinToggleCount[1] > 0 && --pinToggleCount[1] == 0) forwardSound(1);
}
