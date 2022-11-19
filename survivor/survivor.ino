#include <cglab.h>

EXTERNC void setupGameSurvivor(void);

void setup() {
  md_initMachine();
  setupGameSurvivor();
  initGame();
}

void loop() {
  if (md_nextFrame()) {
    updateFrame();
    md_refresh();
  }
}
