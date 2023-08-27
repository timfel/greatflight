#ifndef ACTIONS_H
#define ACTIONS_H

#include "include/units.h"
#include "ace/types.h"

void actionDo(Unit *unit, UBYTE **map);
void actionMoveTo(Unit *unit, tUbCoordYX goal);

#endif
