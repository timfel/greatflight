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
    ActionRepair,
    ActionTrain,
    ActionResearch,
    ActionDie
} ActionType;
_Static_assert(sizeof(ActionType) == sizeof(UBYTE), "unit stats is not 1 byte");

typedef struct __attribute__((__packed__)) {
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
_Static_assert(sizeof(Action) == sizeof(UBYTE) * 5, "action is too big");

typedef struct _unit Unit;

void actionDo(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]);
void actionMoveTo(Unit *unit, tUbCoordYX goal);
void actionStop(Unit *unit);

#endif
