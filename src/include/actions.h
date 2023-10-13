#ifndef ACTIONS_H
#define ACTIONS_H

#include "include/map.h"
#include "ace/types.h"

typedef enum __attribute__ ((__packed__)) {
    ActionStill,
    ActionMove,
    ActionStop,
    ActionAttackMove,
    ActionAttackTarget,
    ActionHarvest,
    ActionCast,
    ActionBuild,
    ActionBeingBuilt,
    ActionDie
} ActionType;
_Static_assert(sizeof(ActionType) == sizeof(UBYTE), "unit stats is not 1 byte");

typedef struct {
    ActionType action;
    union {
        ULONG ulActionData;
        struct {
            UWORD uwActionDataA;
            UWORD uwActionDataB;
        };
        struct {
            UBYTE ubActionDataA;
            UBYTE ubActionDataB;
            UBYTE ubActionDataC;
            UBYTE ubActionDataD;
        };
    };
} Action;

typedef struct _unit Unit;

void actionDo(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]);
void actionMoveTo(Unit *unit, tUbCoordYX goal);

#endif
