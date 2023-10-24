#include "include/buildings.h"
#include "include/icons.h"
#include "include/actions.h"
#include "game.h"

tBuildingManager g_BuildingManager;

BuildingType BuildingTypes[] = {
    [BUILDING_HUMAN_TOWNHALL] = {
        .iconIdx = ICON_HHALL,
        .tileIdx = 44,
        .size = 4,
        .stats = {
            .maxHP = 1200,
        },
        .costs = {
            .gold = 1200,
            .wood = 800,
            .hpIncrease = 2,
        },
    },
    [BUILDING_HUMAN_FARM] = {
        .iconIdx = ICON_HFARM,
        .tileIdx = 23,
        .size = 2,
        .stats = {
            .maxHP = 400,
        },
        .costs = {
            .gold = 500,
            .wood = 250,
            .hpIncrease = 1,
        },
    },
    [BUILDING_HUMAN_BARRACKS] = {
        .iconIdx = ICON_HBARRACKS,
        .tileIdx = 24,
        .size = 4,
        .stats = {
            .maxHP = 800,
        },
        .costs = {
            .gold = 700,
            .wood = 450,
            .hpIncrease = 1,
        },
    },
    [BUILDING_HUMAN_LUMBERMILL] = {
        .iconIdx = ICON_HLUMBERMILL,
        .tileIdx = 28,
        .size = 4,
        .stats = {
            .maxHP = 600,
        },
        .costs = {
            .gold = 600,
            .wood = 450,
            .hpIncrease = 1,
        },
    },
    [BUILDING_HUMAN_SMITHY] = {
        .iconIdx = ICON_HSMITHY,
        .tileIdx = 21,
        .size = 2,
        .stats = {
            .maxHP = 750,
        },
        .costs = {
            .gold = 800,
            .wood = 450,
            .hpIncrease = 1,
        },
    },
    [BUILDING_HUMAN_STABLES] = {
        .iconIdx = 0,
        .tileIdx = 36,
        .size = 4,
        .stats = {
            .maxHP = 500,
        },
        .costs = {
            .gold = 1000,
            .wood = 300,
            .hpIncrease = 0,
        },
    },
    [BUILDING_HUMAN_CHURCH] = {
        .iconIdx = 0,
        .tileIdx = 32,
        .size = 4,
        .stats = {
            .maxHP = 700,
        },
        .costs = {
            .gold = 900,
            .wood = 500,
            .hpIncrease = 1,
        },
    },
    [BUILDING_HUMAN_TOWER] = {
        .iconIdx = 0,
        .tileIdx = 48,
        .size = 2,
        .stats = {
            .maxHP = 500,
        },
        .costs = {
            .gold = 1000,
            .wood = 200,
            .hpIncrease = 1,
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

UBYTE buildingNew(BuildingTypeIndex typeIdx, tUbCoordYX loc) {
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

    // place it on the pathmap
    UBYTE sz = type->size;
    for (UBYTE x = 0; x < sz; ++x) {
        for (UBYTE y = 0; y < sz; ++y) {
            g_Map.m_ubPathmapXY[loc.ubX + x][loc.ubY + y] = MAP_UNWALKABLE_FLAG;
        }
    }

    // place construction site
    UBYTE ubTileX = loc.ubX / TILE_SIZE_FACTOR;
    UBYTE ubTileY = loc.ubY / TILE_SIZE_FACTOR;
    UBYTE buildingTileIdx = sz == 2 ? TILEINDEX_CONSTRUCTION_SMALL : TILEINDEX_CONSTRUCTION_LARGE;
    for (UBYTE y = 0; y < sz / TILE_SIZE_FACTOR; ++y) {
        for (UBYTE x = 0; x < sz / TILE_SIZE_FACTOR; ++x) {
            g_Map.m_ulTilemapXY[ubTileX + x][ubTileY + y] = tileIndexToTileBitmapOffset(buildingTileIdx++);
        }
    }
    return id;
}

void buildingDestroy(Building *) {
    // TODO
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
    UBYTE count = g_BuildingManager.count;
    while (count--) {
        buildingDo(building);
        building++;
    }
}

Building *buildingManagerBuildingAt(tUbCoordYX tile) {
    for (UBYTE i = 0; i < g_BuildingManager.count; ++i) {
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
