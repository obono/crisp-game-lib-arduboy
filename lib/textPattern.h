#ifndef TEXT_PATTERN_H
#define TEXT_PATTERN_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#include "cglab.h"

EXTERNC const uint8_t textPatterns[TEXT_PATTERN_COUNT][CHARACTER_WIDTH];

#endif
