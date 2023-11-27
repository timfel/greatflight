#ifndef UTILS_H
#define UTILS_H

#include <ace/types.h>

UBYTE fast2dDistance(tUbCoordYX a, tUbCoordYX b);

#ifdef ACE_DEBUG
#define assert(condition, ...) do { if (!(condition)) { logWrite(__VA_ARGS__); asm("illegal"); } } while (0)
#else
#define assert(condition, ...)
#endif

#endif
