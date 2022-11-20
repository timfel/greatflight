#ifndef ACTIONS_H
#define ACTIONS_H

#include "units.h"
#include "ace/types.h"

void actionDo(Unit *unit, uint8_t **map);
void actionMoveTo(Unit *unit, tUbCoordYX goal);

#endif
