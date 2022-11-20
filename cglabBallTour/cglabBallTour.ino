#include <cglab.h>

EXTERNC void setupGameBallTour(void);

void setup() {
  setupGameBallTour();
  initGame();
}

void loop() {
  updateGame();
}
