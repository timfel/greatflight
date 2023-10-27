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
            union {
                struct __attribute__((__packed__)) {
                    UBYTE ubTargetY;
                    UBYTE ubTargetX;
                };
                tUbCoordYX target;
            };
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
                    unsigned u6BuildingType:6; // 20 buildings, at most 64
                    unsigned u2State:2; // going to goal, groundwork, waiting
                };
                struct __attribute__((__packed__)) {
                    unsigned u6buildingHPIncrease:2;
                    unsigned u6buildingHPWait:4;
                    unsigned __u2State:2;
                };
            };
        } build;
        struct __attribute__((__packed__)) {    
            UWORD uwTimeLeft;
            unsigned u5UnitType1:5;
            unsigned u5UnitType2:5;
            unsigned u5UnitType3:5;
        } train;
        struct __attribute__((__packed__)) {
            UBYTE ubTimeout;
        } die;
    };
    ActionType action;
} Action;
_Static_assert(sizeof(Action) == sizeof(UBYTE) * 5, "action is too big");

typedef struct _unit Unit;
typedef struct _building Building;

void actionDo(Unit *unit);
void actionMoveTo(Unit *unit, tUbCoordYX goal);
void actionStop(Unit *unit);
void actionBuildAt(Unit *unit, tUbCoordYX goal, UBYTE tileType);

void buildingDo(Building *building);

#endif
