#include <cglab.h>
#include "soundPattern.h"

PROGMEM static const char title[] = "SURVIVOR";
PROGMEM static const char description[] = "[Button] Jump\0";

PROGMEM static const CharacterData characters[] = {
  {{ 0x0F, 0x3F, 0x09, 0x0F, 0x19, 0x1F }, { 0, 0, 6, 6 }},
  {{ 0x1F, 0x1F, 0x09, 0x0F, 0x39, 0x0F }, { 0, 0, 6, 6 }},
  {{ 0x3C, 0x3F, 0x24, 0x3C, 0x27, 0x3C }, { 0, 0, 6, 6 }},
};

typedef struct _Player {
  Vector pos;
  Vector vel;
  uint8_t isOnFloor:1;
  uint8_t isJumping:1;
  uint8_t isJumped:1;
  uint8_t isAlive:1;
  uint8_t ticks:4;
  struct _Player *underFoot;
  struct _Player *onHead;
} Player;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} DownedPlayer;

typedef struct {
  Vector pos;
  float vx;
  float r;
  float angle;
  bool isAlive;
} Barrel;

#define SURVIVOR_PLAYER_COUNT 6
static Player players[SURVIVOR_PLAYER_COUNT];
static DownedPlayer downedPlayers[SURVIVOR_PLAYER_COUNT];
static Barrel barrel;

static void initPlayers() {
  FOR_EACH(players, i) {
    players[i].isAlive = false;
    downedPlayers[i].isAlive = false;
  }
}

static void addPlayer() {
  FOR_EACH(players, i) {
    ASSIGN_ARRAY_ITEM(players, i, Player, pl);
    if (!pl->isAlive) {
      vectorSet(&pl->pos, rnd(10, 40), rnd(-9, 9));
      vectorSet(&pl->vel, 0, 0);
      pl->isOnFloor = false;
      pl->isJumping = false;
      pl->isJumped = false;
      pl->underFoot = NULL;
      pl->onHead = NULL;
      pl->ticks = rndi(0, 16);
      pl->isAlive = true;
      break;
    }
  }
}

static void addPlayers() {
  play(POWER_UP);
  FOR_EACH(players, i) { addPlayer(); }
}

static void addDownedPlayer(Vector pos, float vx, float vy) {
  FOR_EACH(downedPlayers, i) {
    ASSIGN_ARRAY_ITEM(downedPlayers, i, DownedPlayer, dp);
    if (!dp->isAlive) {
      dp->pos = pos;
      vectorSet(&dp->vel, vx, vy);
      dp->isAlive = true;
      break;
    }
  }
}

static void update() {
  if (!ticks) {
    initPlayers();
    barrel.isAlive = false;
  }
  if (!barrel.isAlive) {
    addPlayers();
    float r = rnd(5, 16);
    vectorSet(&barrel.pos, 148 + r, 63 - r);
    barrel.vx = rnd(1, 2) / sqrt(r * 0.3 + 1);
    barrel.r = r;
    barrel.angle = rnd(0, M_PI * 2);
    barrel.isAlive = true;
  }
  barrel.pos.x -= barrel.vx * difficulty;
  thickness = 3 + barrel.r * 0.1;
  arc(VEC_XY(barrel.pos), barrel.r, barrel.angle, barrel.angle + M_PI * 2);
  barrel.angle -= barrel.vx / barrel.r;
  particle(barrel.pos.x, barrel.pos.y + barrel.r, barrel.r * 0.05,
           barrel.vx * 5, -0.2, 0.2);
  rect(0, 63, 128, 7);
  int addingPlayerCount = 0;
  FOR_EACH(players, i) {
    ASSIGN_ARRAY_ITEM(players, i, Player, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->underFoot == NULL) {
      FOR_EACH(players, j) {
        ASSIGN_ARRAY_ITEM(players, j, Player, ap);
        SKIP_IS_NOT_ALIVE(ap);
        if (i != j && p->isOnFloor &&
            distanceTo(&p->pos, VEC_XY(ap->pos)) < 4) {
          play(SELECT);
          Player *bp = p;
          for (int k = 0; k < 99; k++) {
            if (bp->underFoot == NULL) {
              break;
            }
            bp = bp->underFoot;
          }
          Player *tp = ap;
          for (int k = 0; k < 99; k++) {
            if (tp->onHead == NULL) {
              break;
            }
            tp = tp->onHead;
          }
          tp->onHead = bp;
          bp->underFoot = tp;
          Player *rp = p;
          for (int k = 0; k < 99; k++) {
            rp->isJumped = rp->isOnFloor = false;
            if (rp->onHead == NULL) {
              break;
            }
            rp = rp->onHead;
          }
          rp = p;
          for (int k = 0; k < 99; k++) {
            rp->isJumped = rp->isOnFloor = false;
            if (rp->underFoot == NULL) {
              break;
            }
            rp = rp->underFoot;
          }
        }
      }
    }
    if (btnp(INPUT_UP | INPUT_A | INPUT_B) &&
        (p->isOnFloor || (p->underFoot != NULL && p->underFoot->isJumped))) {
      play(JUMP);
      vectorSet(&p->vel, 0, -1.25);
      particle(VEC_XY(p->pos), 5, 2, M_PI / 2, 0.5);
      p->isOnFloor = false;
      p->isJumping = true;
      if (p->underFoot != NULL) {
        p->underFoot->onHead = NULL;
        p->underFoot = NULL;
      }
    }
    if (p->underFoot != NULL) {
      p->pos = p->underFoot->pos;
      p->pos.y -= 6;
      vectorSet(&p->vel, 0, 0);
    } else {
      vectorAdd(&(p->pos), p->vel.x * difficulty, p->vel.y * difficulty);
      p->vel.x *= 0.95;
      if ((p->pos.x < 7 && p->vel.x < 0) || (p->pos.x >= 77 && p->vel.x > 0)) {
        p->vel.x *= -0.5;
      }
      if (p->pos.x < 50) {
        p->vel.x += 0.01 * sqrt(50 - p->pos.x + 1);
      } else {
        p->vel.x -= 0.01 * sqrt(p->pos.x - 50 + 1);
      }
      if (p->isOnFloor) {
        if (p->pos.x < barrel.pos.x) {
          p->vel.x -=
              (0.1 * sqrt(barrel.r)) / sqrt(barrel.pos.x - p->pos.x + 1);
        }
      } else {
        p->vel.y += 0.1 * difficulty;
        if (p->pos.y > 60) {
          p->pos.y = 60;
          p->isOnFloor = true;
          p->isJumped = false;
          p->vel.y = 0;
        }
      }
      if (p->pos.y < 0 && p->vel.y < 0) {
        p->vel.y *= -0.5;
      }
    }
    char pc = 'a' + (((ticks + p->ticks * 2) >> 5) & 1);
    Collision cl = character(pc, VEC_XY(p->pos));
    if (colRect(&cl, DEFAULT_COLOR)) {
      if (p->onHead != NULL) {
        p->onHead->underFoot = NULL;
        p->onHead->isJumping = true;
      }
      if (p->underFoot != NULL) {
        p->underFoot->onHead = NULL;
      }
      play(HIT);
      addDownedPlayer(p->pos, p->vel.x - barrel.vx * 2, p->vel.y);
      p->isAlive = false;
      continue;
    }
    if (p->pos.x < 0 || p->pos.x > 128 || p->pos.y < -50 || p->pos.y > 70) {
      addingPlayerCount++;
      p->isAlive = false;
      continue;
    }
  }
  TIMES(addingPlayerCount, i) { addPlayer(); }
  FOR_EACH(players, i) {
    ASSIGN_ARRAY_ITEM(players, i, Player, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->isJumping) {
      p->isJumped = true;
      p->isJumping = false;
    }
  }
  FOR_EACH(downedPlayers, i) {
    ASSIGN_ARRAY_ITEM(downedPlayers, i, DownedPlayer, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x += p->vel.x;
    p->pos.y += p->vel.y;
    p->vel.y += 0.2;
    character('c', VEC_XY(p->pos));
    p->isAlive = p->pos.y < 75;
  }
  COUNT_IS_ALIVE(players, playerCount);
  if (playerCount == 0) {
    play(RANDOM);
    gameOver();
  }
  if (barrel.pos.x < -barrel.r) {
    barrel.isAlive = false;
    addScore(playerCount, 10, 50);
  }
}

void setupGameSurvivor(void) {
  setupGame(title, description, characters, update, soundPatterns);
}
