#include <cglab.h>

EXTERNC void setupGamePakuPaku(void);

void setup() {
  setupGamePakuPaku();
  initGame();
}

void loop() {
  updateGame();
}
