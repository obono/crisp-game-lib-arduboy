#include <cglab.h>

EXTERNC void setupGamePinClimb(void);

void setup() {
  setupGamePinClimb();
  initGame();
}

void loop() {
  updateGame();
}
