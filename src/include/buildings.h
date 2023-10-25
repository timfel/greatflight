#ifndef BUILDINGS
#define BUILDINGS

#include "include/actions.h"
#include <ace/types.h>

typedef enum __attribute__((__packed__)) {
    BUILDING_HUMAN_TOWNHALL,
    BUILDING_HUMAN_FARM,
    BUILDING_HUMAN_BARRACKS,
    BUILDING_HUMAN_LUMBERMILL,
    BUILDING_HUMAN_SMITHY,
    BUILDING_HUMAN_STABLES,
    BUILDING_HUMAN_CHURCH,
    BUILDING_HUMAN_TOWER,
    BUILDING_GOLD_MINE,
    BUILDING_TYPE_COUNT,
} BuildingTypeIndex;
_Static_assert(sizeof(BuildingTypeIndex) == sizeof(UBYTE), "not 1 byte");

typedef struct {
    const char *name;
    UBYTE iconIdx;
    UBYTE tileIdx;
    UBYTE size;
    struct {
        UWORD maxHP;
    } stats;
    struct {
        UWORD gold;
        UWORD wood;
        UBYTE hpIncrease;
    } costs;
} BuildingType;

typedef struct _building {
    BuildingTypeIndex type;
    tUbCoordYX loc;
    Action action;
    UWORD hp;
    tUbCoordYX rallyPoint;
    UBYTE owner;
} Building;

extern BuildingType BuildingTypes[];

/* The maximum number of buildings the game will allocate memory for. */
#define MAX_BUILDINGS 64

typedef struct _buildingmanager {
    Building building[MAX_BUILDINGS];
    UBYTE freeStack[MAX_BUILDINGS];
    UBYTE count;
} tBuildingManager;

extern tBuildingManager g_BuildingManager;

/* Check if building can be at loc in global map. Ignore the origin if that flag is set */
UBYTE buildingCanBeAt(BuildingTypeIndex type, tUbCoordYX loc, UBYTE ignoreOrigin);

/* Create a new building construction at location, return the new building id */
UBYTE buildingNew(BuildingTypeIndex type, tUbCoordYX loc, UBYTE owner);

void buildingDestroy(Building *building);

void buildingManagerInitialize(void);
void buildingManagerProcess(void);
void buildingManagerDestroy(void);

Building *buildingManagerBuildingAt(tUbCoordYX tile);

#endif
