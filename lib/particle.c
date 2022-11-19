#include <math.h>

#include "cglab.h"
#include "machineDependent.h"
#include "vector.h"

typedef struct {
  uint8_t x;
  uint8_t y:7;
  uint8_t color:1;
  float length;
  uint16_t ticks:6;
  uint16_t speed:4;
  uint16_t angle:6;
} Particle;

#define MAX_PARTICLE_COUNT 16
static Particle particles[MAX_PARTICLE_COUNT];
static uint8_t particleIndex = 0;

void initParticle() {
  for (uint8_t i = 0; i < MAX_PARTICLE_COUNT; i++) {
    particles[i].ticks = 0;
  }
}

void addParticle(float x, float y, float count, float speed, float angle,
                 float angleWidth) {
  if (color == TRANSPARENT ||
      x < 0 || y < 0 || x >= VIEW_SIZE_X || y >= VIEW_SIZE_Y) {
    return;
  }
  if (count < 1) {
    if (rnd(0, 1) > count) {
      return;
    }
    count = 1;
  }
  for (uint8_t i = 0; i < count; i++) {
    Particle *p = &particles[particleIndex];
    p->x = x;
    p->y = y;
    p->color = (color >= LIGHT1);
    p->length = 0;
    p->ticks = clamp(rnd(10, 20) + sqrtf(fabsf(speed)), 10, 60);
    float s = (sqrtf(speed * rnd(0.5f, 1) * 2048.0f) - p->ticks) / 16.0f;
    p->speed = clamp(s, 0, 15);
    float a = angle + rnd(0, angleWidth) - angleWidth / 2;
    p->angle = (uint16_t)(a * 32.0f / M_PI + 0.5f) & 0x3F;
    particleIndex++;
    if (particleIndex >= MAX_PARTICLE_COUNT) {
      particleIndex = 0;
    }
  }
}

void updateParticles() {
  for (uint8_t i = 0; i < MAX_PARTICLE_COUNT; i++) {
    Particle *p = &particles[i];
    if (p->ticks == 0) {
      continue;
    }
    float v = p->speed * 16.0f + p->ticks;
    p->length += (v * v) / 2048.0f;
    p->ticks--;
    Vector pos;
    vectorSet(&pos, p->x, p->y);
    addWithAngle(&pos, p->angle * M_PI / 32.0, p->length);
    md_drawPixel(VEC_XY(pos), p->color ? INVERT1 : DARK1);
  }
}
