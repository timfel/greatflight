#ifndef BUILDINGS
#define BUILDINGS

#include "include/actions.h"
#include "include/units.h"
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
        UBYTE hpShift;
        HPBase baseHp;
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
    UBYTE id;
    BYTE owner;
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

/* Create a new building construction at location, return the new building */
Building *buildingNew(BuildingTypeIndex type, tUbCoordYX loc, UBYTE owner);

void buildingDestroy(Building *building);

static inline UWORD buildingTypeMaxHealth(BuildingType *type) {
    return type->stats.baseHp << type->stats.hpShift;
}

void buildingFinishBuilding(Building *building);

void buildingManagerInitialize(void);
void buildingManagerProcess(void);
void buildingManagerDestroy(void);

Building *buildingById(UBYTE id);

Building *buildingManagerFindBuildingByTypeAndPlayerAndLocation(BuildingTypeIndex typeIdx, UBYTE ownerIdx, tUbCoordYX loc);

Building *buildingManagerBuildingAt(tUbCoordYX tile);

void buildingsLoad(tFile *map);

#endif
