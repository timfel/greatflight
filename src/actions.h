#ifndef ACTIONS_H
#define ACTIONS_H

#include "include/units.h"
#include "ace/types.h"

void actionDo(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]);
void actionMoveTo(Unit *unit, tUbCoordYX goal);

#endif
