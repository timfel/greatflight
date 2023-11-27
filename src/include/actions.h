#ifndef ACTIONS_H
#define ACTIONS_H

#include "include/map.h"
#include "ace/types.h"

/*
 * Many actions use ActionMove to move to a goal and store
 * themselves in the follow up action data.
 * Action move must be interruptible by any action though.
 * So move is only interrupted by action ids lower than move.
 * When actions use move but don't want to be interrupted by
 * move themselves, they set the high bit and thus can store their
 * data in the nextAction field of the unit.
 */
#define ACTION_AFTER_MOVE(action) ((action) | 0x80)
#define ACTION_WITHOUT_MOVE(action) ((action) & 0x7F)
#define ACTION_IS_AFTER_MOVE(action) ((action) & 0x80)
typedef enum __attribute__ ((__packed__)) {
    ActionStill,
    ActionStop,
    ActionAttackMove,
    ActionAttackUnit,
    ActionAttackBuilding,
    ActionFollow,
    ActionHarvestTerrain,
    ActionHarvestMine,
    ActionCast,
    ActionBuild,
    ActionRepair,
    ActionDie,
    ActionBeingBuilt,
    ActionTrain,
    ActionResearch,
    ActionMove,
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
            tUbCoordYX target;
            UBYTE ubRetries;
            unsigned u4Wait:4;
        } move;
        struct __attribute__((__packed__)) {
            UBYTE ubState;
            union __attribute__((__packed__)) {
                // depending on state this stores the desired type
                // or the new building id
                UBYTE ubBuildingID;
                UBYTE ubBuildingType;
            };
            union {
                tUbCoordYX target;
                struct __attribute__((__packed__)) {
                    UBYTE ubBuildingHPIncrease;
                    UBYTE ubBuildingHPWait;
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
        struct __attribute__((__packed__)) {
            unsigned u4State:4;
            unsigned u4Direction:4;
            UBYTE ubWait;
            union {
                tUbCoordYX lastHarvestLocation;
                struct {
                    UBYTE lastMineId;
                    UBYTE lastDepotId;
                };
            };
        } harvest;
        struct __attribute__((__packed__)) {
            union {
                UBYTE ubUnitId;
                UBYTE ubBuildingId;
                tUbCoordYX goal;
            };
            UBYTE ubWait;
        } attack;
        struct __attribute__((__packed__)) {
            UBYTE ubUnitId;
            UBYTE ubWait;
        } follow;
        struct __attribute__((__packed__)) {
            UBYTE ubBuildingId;
            UBYTE ubWait;
        } repair;
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
void actionHarvestAt(Unit *unit, tUbCoordYX goal);
void actionHarvestTile(Unit *unit, tUbCoordYX goal);
void actionHarvestMine(Unit *unit, Building *mine);
void actionAttackAt(Unit *unit, tUbCoordYX goal);
void actionAttackUnit(Unit *unit, Unit *target);
void actionAttackBuilding(Unit *unit, Building *target);
void actionFollow(Unit *unit, Unit *target);
void actionRepair(Unit *unit, Building *target);

void buildingDo(Building *building);

#endif
