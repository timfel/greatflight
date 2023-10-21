#ifndef ACTIONS_H
#define ACTIONS_H

#include "include/map.h"
#include "include/buildings.h"
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
        struct __attribute__((__packed__)) {
            UWORD uwActionDataA;
            UWORD uwActionDataB;
        };
        struct __attribute__((__packed__)) {
            UBYTE ubActionDataA;
            UBYTE ubActionDataB;
            UBYTE ubActionDataC;
            UBYTE ubActionDataD;
        };
        struct __attribute__((__packed__)) {
            UBYTE ubWait;
        } still;
        struct __attribute__((__packed__)) {
            UBYTE ubTargetX;
            UBYTE ubTargetY;
            unsigned u4Wait:4;
            unsigned u4Retries:4;
            UBYTE unused;
        } move;
        struct __attribute__((__packed__)) {
            UBYTE __move1;
            UBYTE __move2;
            union {
                UBYTE __move3;
                UBYTE ubBuildingID;
            };
            union {
                struct __attribute__((__packed__)) {
                    unsigned u5BuildingType:5; // 20 buildings, at most 32
                    unsigned u2State:3; // going to goal, groundwork, waiting
                };
                struct __attribute__((__packed__)) {
                    unsigned u5buildingHPIncrease:5;
                    unsigned __u2State:3; // going to goal, groundwork, waiting
                };
            };
        } build;
    };
} Action;
_Static_assert(sizeof(Action) == sizeof(UBYTE) * 5, "action is too big");

typedef struct _unit Unit;

void actionDo(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]);
void actionMoveTo(Unit *unit, tUbCoordYX goal);
void actionStop(Unit *unit);
void actionBuildAt(Unit *unit, tUbCoordYX goal, BuildingType tileType);

#endif
