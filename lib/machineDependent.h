#ifndef MACHINE_DEPENDENT_H
#define MACHINE_DEPENDENT_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#define FPS 60

#define VIEW_SIZE_X 128
#define VIEW_SIZE_Y 64

#define CHARACTER_WIDTH 6
#define CHARACTER_HEIGHT 6

#define INPUT_LEFT  bit(5)
#define INPUT_RIGHT bit(6)
#define INPUT_UP    bit(7)
#define INPUT_DOWN  bit(4)
#define INPUT_A     bit(3)
#define INPUT_B     bit(2)

EXTERNC void md_initMachine(void);
EXTERNC bool md_nextFrame(void);
EXTERNC void md_refresh(void);

EXTERNC void md_clearView(void);
EXTERNC void md_drawPixel(float x, float y, int8_t color);
EXTERNC void md_drawRect(float x, float y, float w, float h, int8_t color);
EXTERNC void md_drawCharacter(const uint8_t grid[CHARACTER_WIDTH],
                              float x, float y, int8_t color);
EXTERNC uint8_t md_getInputState(void);

EXTERNC bool md_getSoundEnabled(void);
EXTERNC void md_setSoundEnabled(bool isEnabled);
EXTERNC void md_saveSoundEnabled(void);
EXTERNC void md_playSound(const uint8_t *sound, bool isBgm);
EXTERNC void md_stopSound(bool isBgm);

#endif
