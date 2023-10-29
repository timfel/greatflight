#ifndef MAP_H
#define MAP_H

#include "ace/types.h"
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
    ULONG m_ulTilemapXY[MAP_SIZE][MAP_SIZE];
    UBYTE m_ubPathmapXY[PATHMAP_SIZE][PATHMAP_SIZE];
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

extern void mapLoad(tFile *file, void(*loadTileBitmap)());

#define MAP_UNWALKABLE_FLAG 0b1
#define MAP_GROUND_FLAG    0b10
#define MAP_FOREST_FLAG   0b101
#define MAP_WATER_FLAG   0b1001
#define MAP_COAST_FLAG  0b10000

static inline UBYTE tileIsWalkable(UBYTE tile) {
    return !(tile & MAP_UNWALKABLE_FLAG);
}

static inline UBYTE mapIsWalkable(UBYTE x, UBYTE y) {
    return x < PATHMAP_SIZE && y < PATHMAP_SIZE && tileIsWalkable(g_Map.m_ubPathmapXY[x][y]);
}

static inline UBYTE tileIsHarvestable(UBYTE tile) {
    return tile & MAP_FOREST_FLAG;
}

static inline UBYTE mapIsHarvestable(UBYTE x, UBYTE y) {
    return tileIsHarvestable(g_Map.m_ubPathmapXY[x][y]);
}

static inline void markMapTile(UBYTE x, UBYTE y) {
    g_Map.m_ubPathmapXY[x][y] |= MAP_UNWALKABLE_FLAG;
}

static inline void unmarkMapTile(UBYTE x, UBYTE y) {
    g_Map.m_ubPathmapXY[x][y] ^= MAP_UNWALKABLE_FLAG;
}

/*
 * 
 * dbu_ ____ tttt tttt - (d)iscovered, 
 * 
 * 
 * 11bb bbqq qqii iiii - 4x4 (b)uilding id (gold, barracks, mill, Stable, Hall, Church = 11), (q)uadrant of building, unitlist (i)ndex
 * 101b bbqq qqii iiii - 3x3 (b)uilding id (smith, tower, farm), (q)uadrant of building, unitlist (i)ndex
 * 1001 bqqq qqii iiii - 5x5 (b)uilding id (blackrock, stormwind), (q)uadrant of building, unitlist (i)ndex
 * 01uu uuuu iiii iiii - (u)nused, unit (i)d
 * 00wb dhau tttt tttt - (w)alkable, (b)uildable, (d)iscovered, (h)arvestable, (a)ttackable, (u)nused, original (t)ile from map data
 */

typedef UWORD* tileInfo;

#define tileInfoIsBuildingMacro(x) BTST(*x, 0)
#define tileInfoIsUnitMacro(x) BTST(*x, 1)
#define tileInfoIsDiscovered(x) BTST(*x, 0)

struct tileInfoFull {
    unsigned isBuilding:1;
    unsigned isUnit:1;
    union {
        struct {
            unsigned is4x4:1;
            unsigned is3x3:1;
            unsigned is5x5:1;
        } size;
        struct {
            unsigned padding:1;
            unsigned type:4;
            unsigned quadrant:4;
            unsigned index:6;
        } four;
        struct {
            unsigned padding:2;
            unsigned type:3;
            unsigned quadrant:4;
            unsigned index:6;
        } three;
        struct {
            unsigned padding:3;
            unsigned type:1;
            unsigned quadrant:5;
            unsigned index:6;
        } five;
    } building;
    struct {
        unsigned padding:6;
        unsigned index:8;
    } unit;
    struct {
        unsigned isWalkable:1;
        unsigned isBuildable:1;
        unsigned isDiscovered:1;
        unsigned isHarvestable:1;
        unsigned isAttackable:1;
        unsigned unused:1;
        unsigned originalTileIndex:8;
    } info;
};

#endif
