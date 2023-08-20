#ifndef MAP_H
#define MAP_H

#include "ace/types.h"

#define MAPDIR "resources/maps/"

struct Map {
    const char *path;
    const char *tileset;
    UBYTE width;
    UBYTE height;
    UBYTE **tiles;
    UBYTE **pathmap;
};

/**
 * @brief The global structure that holds the map data.
 */
extern struct Map g_Map;

#define TILE_SHIFT 5
#define TILE_SIZE (1 << TILE_SHIFT)
#define TILE_SIZE_WORDS (TILE_SIZE >> 4)
#define TILE_SIZE_BYTES (TILE_SIZE >> 3)
#define TILE_HEIGHT_LINES (TILE_SIZE * BPP)
#define TILE_FRAME_BYTES (TILE_SIZE_BYTES * TILE_HEIGHT_LINES)
#define MAP_SIZE 32

static inline UBYTE mapIsWalkable(UBYTE **map, UBYTE x, UBYTE y) {
    return map[x][y] < 15;
}

static inline UBYTE mapIsHarvestable(UBYTE **map, UBYTE x, UBYTE y) {
    return map[x][y] == 17;
}

static inline void markMapTile(UBYTE **map, UBYTE x, UBYTE y) {
    map[x][y] = ~map[x][y];
}

static inline void unmarkMapTile(UBYTE **map, UBYTE x, UBYTE y) {
    map[x][y] = ~map[x][y];
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
