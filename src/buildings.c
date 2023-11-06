#include "include/buildings.h"
#include "include/icons.h"
#include "include/actions.h"
#include "game.h"

tBuildingManager g_BuildingManager;

BuildingType BuildingTypes[] = {
    [BUILDING_HUMAN_TOWNHALL] = {
        .name = "Townhall",
        .iconIdx = ICON_HHALL,
        .tileIdx = 44,
        .size = 4,
        .stats = {
            .hpShift = 6,
            .baseHp = HP_20,
        },
        .costs = {
            .gold = 1200,
            .wood = 800,
            .hpIncrease = 6,
        },
    },
    [BUILDING_HUMAN_FARM] = {
        .name = "Farm",
        .iconIdx = ICON_HFARM,
        .tileIdx = 23,
        .size = 2,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_25,
        },
        .costs = {
            .gold = 500,
            .wood = 250,
            .hpIncrease = 5,
        },
    },
    [BUILDING_HUMAN_BARRACKS] = {
        .name = "Barracks",
        .iconIdx = ICON_HBARRACKS,
        .tileIdx = 24,
        .size = 4,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_25,
        },
        .costs = {
            .gold = 700,
            .wood = 450,
            .hpIncrease = 5,
        },
    },
    [BUILDING_HUMAN_LUMBERMILL] = {
        .name = "Lumbermill",
        .iconIdx = ICON_HLUMBERMILL,
        .tileIdx = 28,
        .size = 4,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_20,
        },
        .costs = {
            .gold = 600,
            .wood = 450,
            .hpIncrease = 5,
        },
    },
    [BUILDING_HUMAN_SMITHY] = {
        .name = "Smithy",
        .iconIdx = ICON_HSMITHY,
        .tileIdx = 21,
        .size = 2,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_25,
        },
        .costs = {
            .gold = 800,
            .wood = 450,
            .hpIncrease = 5,
        },
    },
    [BUILDING_HUMAN_STABLES] = {
        .name = "Stables",
        .iconIdx = 0,
        .tileIdx = 36,
        .size = 4,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_20,
        },
        .costs = {
            .gold = 1000,
            .wood = 300,
            .hpIncrease = 4,
        },
    },
    [BUILDING_HUMAN_CHURCH] = {
        .name = "Church",
        .iconIdx = 0,
        .tileIdx = 32,
        .size = 4,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_25,
        },
        .costs = {
            .gold = 900,
            .wood = 500,
            .hpIncrease = 5,
        },
    },
    [BUILDING_HUMAN_TOWER] = {
        .name = "Tower",
        .iconIdx = 0,
        .tileIdx = 48,
        .size = 2,
        .stats = {
            .hpShift = 5,
            .baseHp = HP_20,
        },
        .costs = {
            .gold = 1000,
            .wood = 200,
            .hpIncrease = 5,
        }
    },
};

UBYTE buildingCanBeAt(BuildingTypeIndex type, tUbCoordYX loc, UBYTE ignoreOrigin) {
    UBYTE sz = BuildingTypes[type].size;
    for (UBYTE x = 0; x < sz; ++x) {
        for (UBYTE y = 0; y < sz; ++y) {
            if (ignoreOrigin && x == 0 && y == 0) {
                continue;
            }
            if (g_Map.m_ubPathmapXY[loc.ubX + x][loc.ubY + y] != MAP_GROUND_FLAG) {
                return 0;
            }
        }
    }
    return 1;
}

UBYTE buildingNew(BuildingTypeIndex typeIdx, tUbCoordYX loc, UBYTE owner) {
    if (g_BuildingManager.count >= MAX_BUILDINGS) {
        return -1;
    }
    BuildingType *type = &BuildingTypes[typeIdx];
    UBYTE id = g_BuildingManager.freeStack[g_BuildingManager.count++];
    Building *building = &g_BuildingManager.building[id];
    building->action.action = ActionBeingBuilt;
    building->hp = 1;
    building->loc = loc;
    building->rallyPoint = loc;
    building->type = typeIdx;
    building->owner = owner;

    // place construction site
    UBYTE sz = type->size;
    UBYTE buildingTileIdx = sz == 2 ? TILEINDEX_CONSTRUCTION_SMALL : TILEINDEX_CONSTRUCTION_LARGE;
    mapSetGraphicTileRangeSquare(loc.ubX, loc.ubY, sz, buildingTileIdx);
    return id;
}

void buildingDestroy(Building *building) {
    BuildingType *type = &BuildingTypes[building->type];
    tUbCoordYX loc = building->loc;
    UBYTE idx = (ULONG)building - (ULONG)g_BuildingManager.building;
    g_BuildingManager.count--;
    g_BuildingManager.freeStack[g_BuildingManager.count] = idx;

    // replace grass tiles
    mapSetGraphicTileSquare(loc.ubX, loc.ubY, type->size, TILEINDEX_GRASS);

    // make sure it is skipped in action loop
    building->action.action = ActionStill;
    building->type = BUILDING_NONE;
}

void buildingManagerInitialize(void) {
    g_BuildingManager.count = 0;
    for (UBYTE i = 0; i < MAX_BUILDINGS; ++i) {
        g_BuildingManager.freeStack[i] = i;
    }
}

void buildingManagerDestroy() {
    for (UBYTE i = 0; i < g_BuildingManager.count; ++i) {
        buildingDestroy(&g_BuildingManager.building[i]);
    }
}

void buildingManagerProcess(void) {
    Building *building = g_BuildingManager.building;
    UBYTE count = MAX_BUILDINGS;
    while (count--) {
        buildingDo(building);
        building++;
    }
}

UBYTE fast2dDistance(tUbCoordYX a, tUbCoordYX b) {
    // assumes that actual values are maximum 64
    BYTE x = ABS((BYTE)a.ubX - (BYTE)b.ubX);
    BYTE y = ABS((BYTE)a.ubY - (BYTE)b.ubY);
    // this function computes the distance from 0,0 to x,y with 3.5% error
    // found here: https://www.reddit.com/r/askmath/comments/ekzurn/understanding_a_fast_distance_calculation/
    int mn = MIN(x, y);
    return (x + y - (mn / (2 << 1)) - (mn / (2 << 2)) + (mn / (2 << 4)));
}

Building *buildingManagerFindBuildingByTypeAndPlayerAndLocation(BuildingTypeIndex typeIdx, UBYTE ownerIdx, tUbCoordYX loc) {
    Building *building = g_BuildingManager.building;
    UBYTE count = MAX_BUILDINGS;
    Building *bestBuilding = NULL;
    UBYTE dist = 0xFF;
    while (count--) {
        if (building->owner == ownerIdx && building->type == typeIdx) {
            if (fast2dDistance(building->loc, loc) < dist) {
                bestBuilding = building;
            }
        }
        building++;
    }
    return bestBuilding;
}

Building *buildingManagerBuildingAt(tUbCoordYX tile) {
    for (UBYTE i = 0; i < MAX_BUILDINGS; ++i) {
        Building *building = &g_BuildingManager.building[i];
        UBYTE sz = BuildingTypes[building->type].size;
        tUbCoordYX loc = building->loc;
        if (tile.ubX >= loc.ubX && tile.ubX <= loc.ubX + sz &&
	            tile.ubY >= loc.ubY && tile.ubY <= loc.ubY + sz) {
            return building;
        }
    }
    return NULL;
}
