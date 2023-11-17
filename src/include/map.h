#ifndef MAP_H
#define MAP_H

#include "limits.h"
#include "ace/types.h"
#include "ace/macros.h"
#include "ace/managers/log.h"
#include "ace/utils/file.h"

#define MAPDIR "resources/maps/"

#define TILE_SHIFT 5
#define TILE_SIZE (1 << TILE_SHIFT)
#define TILE_SIZE_WORDS (TILE_SIZE >> 4)
#define TILE_SIZE_BYTES (TILE_SIZE >> 3)
#define TILE_HEIGHT_LINES (TILE_SIZE * BPP)
#define TILE_FRAME_BYTES (TILE_SIZE_BYTES * TILE_HEIGHT_LINES)
#define MAP_SIZE 32
#define PATHMAP_SIZE (MAP_SIZE * 2)
#define PATHMAP_TILE_SIZE (TILE_SIZE / 2)
#define TILE_SIZE_FACTOR (TILE_SIZE / PATHMAP_TILE_SIZE)

struct Map {
    const char *m_pName;
    const char *m_pTileset;
    UBYTE m_ubTilemapXY[MAP_SIZE][MAP_SIZE];
    ULONG m_ulVisibleMapXY[MAP_SIZE][MAP_SIZE];
    UBYTE m_ubPathmapXY[PATHMAP_SIZE][PATHMAP_SIZE];
    UBYTE m_ubUnitCacheXY[PATHMAP_SIZE][PATHMAP_SIZE];
};

enum __attribute__((__packed__)) TileIndices {
    TILEINDEX_GRASS = 0,
    TILEINDEX_WOOD_REMOVED = 5,
    TILEINDEX_WOOD_SMALL = 6,
    TILEINDEX_WOOD_MEDIUM = 7,
    TILEINDEX_WOOD_LARGE = 8,
    TILEINDEX_CONSTRUCTION_SMALL = 22,
    TILEINDEX_CONSTRUCTION_LARGE = 40,
};

/**
 * @brief The global structure that holds the map data.
 */
extern struct Map g_Map;

extern void mapLoad(tFile *file);

#define MAP_UNWALKABLE_FLAG (1 << 0)
#define MAP_GROUND_FLAG     (1 << 1)
#define MAP_FOREST_FLAG     (1 << 2)
#define MAP_WATER_FLAG      (1 << 3)
#define MAP_COAST_FLAG      (1 << 4)
#define MAP_BUILDING_FLAG   (1 << 5)
#define MAP_GOLDMINE_FLAG   (1 << 6)
#define MAP_OWNER_BIT       (1 << 7)

// for now we only support two players, so it's 1 bit
#define MAP_OWNER_FLAGS(owner) ((owner) == 2 ? MAP_GOLDMINE_FLAG : ((owner) ? MAP_OWNER_BIT : 0))

#define IS_TILE_UNCOVERED(x) ((LONG)(x) & 0xFF000000)

ULONG tileIndexToTileBitmapOffset(UBYTE index);

UBYTE tileBitmapOffsetToTileIndex(ULONG offset);

static inline UBYTE mapGetTileAt(UBYTE x, UBYTE y) {
    return g_Map.m_ubPathmapXY[x][y];
}

static inline UBYTE tileIsWalkable(UBYTE tile) {
    return !(tile & MAP_UNWALKABLE_FLAG);
}

static inline UBYTE mapIsWalkable(UBYTE x, UBYTE y) {
    return x < PATHMAP_SIZE && y < PATHMAP_SIZE && tileIsWalkable(g_Map.m_ubPathmapXY[x][y]);
}

static inline UBYTE tileIsBuilding(UBYTE tile) {
    return tile & MAP_BUILDING_FLAG;
}

static inline UBYTE mapIsBuilding(UBYTE x, UBYTE y) {
    return tileIsBuilding(g_Map.m_ubPathmapXY[x][y]);
}

static inline UBYTE tileIsHarvestable(UBYTE tile) {
    return tile & MAP_FOREST_FLAG;
}

static inline UBYTE mapIsHarvestable(UBYTE x, UBYTE y) {
    return tileIsHarvestable(g_Map.m_ubPathmapXY[x][y]);
}

static inline UBYTE mapIsGround(UBYTE x, UBYTE y) {
    return g_Map.m_ubPathmapXY[x][y] == MAP_GROUND_FLAG;
}

static inline void mapMarkTileOccupied(UBYTE id, UBYTE owner, UBYTE x, UBYTE y) {
    g_Map.m_ubPathmapXY[x][y] |= (MAP_UNWALKABLE_FLAG | (owner ? MAP_OWNER_BIT : 0));
    g_Map.m_ubUnitCacheXY[x][y] = id;
}

static inline void mapUnmarkTileOccupied(UBYTE x, UBYTE y) {
    g_Map.m_ubPathmapXY[x][y] &= ~(MAP_UNWALKABLE_FLAG | MAP_OWNER_BIT);
}

typedef enum {
    SIGHT_SELF,
    SIGHT_ADJACENT,
    SIGHT_ADJACENT_BIG_HOUSE,
    SIGHT_NEAR,
    SIGHT_MEDIUM,
    SIGHT_FAR
} SightRange;

static inline void __incVisCount(UBYTE tileX, UBYTE tileY) {
    ULONG tile = tileIndexToTileBitmapOffset(g_Map.m_ubTilemapXY[tileX][tileY]);
    UBYTE visCount = (UBYTE)(g_Map.m_ulVisibleMapXY[tileX][tileY] >> 24) + 1;
    g_Map.m_ulVisibleMapXY[tileX][tileY] = (tile & 0x00FFFFFF) | (visCount << 24);
}

static inline void __decVisCount(UBYTE tileX, UBYTE tileY) {
    ULONG tile = g_Map.m_ulVisibleMapXY[tileX][tileY];
    UBYTE visCount = (UBYTE)(tile >> 24) - 1;
    g_Map.m_ulVisibleMapXY[tileX][tileY] = (tile & 0x00FFFFFF) | (visCount << 24);
}

typedef void(*visFunc)(UBYTE, UBYTE);

static inline void mapChangeUnitSight(UBYTE x, UBYTE y, SightRange range, visFunc func) {
    // XXX: this hardcodes that our tilemap tiles are twice the size of our pathmap tiles,
    // so we only update unit sights when moving
    switch (range) {
        case SIGHT_SELF: {
            func(x >> 1, y >> 1);
            return;
        case SIGHT_ADJACENT:
            UBYTE tileY = y >> 1;
            UBYTE tileX = x >> 1;
            for (BYTE cY = tileY - 1; cY <= tileY + 1; ++cY) {
                if (cY < 0) {
                    continue;
                }
                if (cY > MAP_SIZE) {
                    break;
                }
                for (BYTE cX = tileX - 1; cX <= tileX + 1; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, cY);
                }
            }
            return;
        case SIGHT_ADJACENT_BIG_HOUSE: {
            UBYTE tileY = y >> 1;
            UBYTE tileX = x >> 1;
            for (BYTE cY = tileY - 1; cY <= tileY + 2; ++cY) {
                if (cY < 0) {
                    continue;
                }
                if (cY > MAP_SIZE) {
                    break;
                }
                for (BYTE cX = tileX - 1; cX <= tileX + 2; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, cY);
                }
            }
            return;
        }
        case SIGHT_NEAR: {
            UBYTE tileY = y >> 1;
            UBYTE tileX = x >> 1;
            if ((BYTE)(tileY - 2) >= 0) {
                func(tileX, tileY - 2);
            }
            if ((BYTE)(tileX - 2) >= 0) {
                func(tileX - 2, tileY);
            }
            for (BYTE cY = tileY - 1; cY <= tileY + 1; ++cY) {
                if (cY < 0) {
                    continue;
                }
                if (cY > MAP_SIZE) {
                    break;
                }
                for (BYTE cX = tileX - 1; cX <= tileX + 1; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, cY);
                }
            }
            if ((BYTE)(tileX + 2) < MAP_SIZE) {
                func(tileX + 2, tileY);
            }
            if ((BYTE)(tileY + 2) < MAP_SIZE) {
                func(tileX, tileY + 2);
            }
            return;
        }
        case SIGHT_MEDIUM: {
            UBYTE tileY = y >> 1;
            UBYTE tileX = x >> 1;
            if ((BYTE)(tileY - 3) >= 0) {
                func(tileX, tileY - 3);
            }
            if ((BYTE)(tileX - 3) >= 0) {
                func(tileX - 3, tileY);
            }
            if ((BYTE)(tileY - 2) >= 0) {
                for (BYTE cX = tileX - 1; cX <= tileX + 1; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, tileY - 2);
                }
            }
            if ((BYTE)(tileX - 2) >= 0) {
                for (BYTE cY = tileY - 1; cY <= tileY + 1; ++cY) {
                    if (cY < 0) {
                        continue;
                    }
                    if (cY > MAP_SIZE) {
                        break;
                    }
                    func(tileX - 2, cY);
                }
            }
            for (BYTE cY = tileY - 1; cY <= tileY + 1; ++cY) {
                if (cY < 0) {
                    continue;
                }
                if (cY > MAP_SIZE) {
                    break;
                }
                for (BYTE cX = tileX - 1; cX <= tileX + 1; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, cY);
                }
            }
            if ((BYTE)(tileY + 2) >= 0) {
                for (BYTE cX = tileX - 1; cX <= tileX + 1; ++cX) {
                    if (cX < 0) {
                        continue;
                    }
                    if (cX > MAP_SIZE) {
                        break;
                    }
                    func(cX, tileY + 2);
                }
            }
            if ((BYTE)(tileX + 2) >= 0) {
                for (BYTE cY = tileY - 1; cY <= tileY + 1; ++cY) {
                    if (cY < 0) {
                        continue;
                    }
                    if (cY > MAP_SIZE) {
                        break;
                    }
                    func(tileX + 2, cY);
                }
            }
            if ((BYTE)(tileX + 3) < MAP_SIZE) {
                func(tileX + 3, tileY);
            }
            if ((BYTE)(tileY + 3) < MAP_SIZE) {
                func(tileX, tileY + 2);
            }
            return;
        }
        case SIGHT_FAR:
            return;
        }
    }
}

static inline void mapMarkUnitSight(UBYTE x, UBYTE y, SightRange range) {
    mapChangeUnitSight(x, y, range, &__incVisCount);
}

static inline void mapUnmarkUnitSight(UBYTE x, UBYTE y, SightRange range) {
    mapChangeUnitSight(x, y, range, &__decVisCount);
}

static inline UBYTE tileGetOwner(UBYTE tile) {
    return (tile & MAP_OWNER_BIT) ? 1 : 0;
}

static inline UBYTE mapGetOwnerAt(UBYTE x, UBYTE y) {
    return tileGetOwner(g_Map.m_ubPathmapXY[x][y]);
}

static inline UBYTE mapGetUnitAt(UBYTE x, UBYTE y) {
    UBYTE tile = g_Map.m_ubPathmapXY[x][y];
    if (tileIsWalkable(tile) || tileIsBuilding(tile)) {
        return -1;
    }
    return g_Map.m_ubUnitCacheXY[x][y];
}

static inline UBYTE mapGetBuildingAt(UBYTE x, UBYTE y) {
    if (!mapIsBuilding(x, y)) {
        return -1;
    }
    return g_Map.m_ubUnitCacheXY[x][y];
}

/*
 * Return the graphical tile at the pathmap position.
 */
UBYTE mapGetGraphicTileAt(UBYTE x, UBYTE y);
/*
 * Set graphical tile at the pathmap position.
 */
void mapSetGraphicTileAt(UBYTE x, UBYTE y, UBYTE tileIndex);

/*
 * Set a graphical tile in a square starting at the pathmap position.
 */
void mapSetGraphicTileSquare(UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE tileIndex);

/*
 * Set a range of graphical tiles in a square starting at the given pathmap position,
 * incrementing the tile index along the way. 
 */
void mapSetBuildingGraphics(UBYTE id, UBYTE owner, UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE firstTileIndex);

/*
 * Increment the tile at the pathmap position by 1.
 */
void mapIncGraphicTileAt(UBYTE x, UBYTE y);
/*
 * Decrement the tile at the pathmap position by 1.
 */
void mapDecGraphicTileAt(UBYTE x, UBYTE y);

#endif
