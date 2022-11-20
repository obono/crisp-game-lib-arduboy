#include <cglab.h>

EXTERNC void setupGameSurvivor(void);

void setup() {
  setupGameSurvivor();
  initGame();
}

void loop() {
  updateGame();
}
