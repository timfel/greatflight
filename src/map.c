#include "include/map.h"
#include "include/resources.h"
#include "game.h"
#include "string.h"

struct Map g_Map;

static void setFlags(UBYTE tileIndex, UBYTE x, UBYTE y) {
    switch (tileIndex) {
        case 0: // grass
        case 1: case 2: case 3: case 4: // bridges
        case 5: // chopped tree
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG;
            break;
        case 6: case 7: case 8: // trees
            g_Map.m_ubPathmapXY[x][y] = MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 9: // water
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 10:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 11:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 12:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            break;
        case 13:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            break;
        case 14:
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 15:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            break;
        case 16:
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            break;
        case 17:
            g_Map.m_ubPathmapXY[x][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 18:
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 19:
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            break;
        case 20:
            g_Map.m_ubPathmapXY[x][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_GROUND_FLAG | MAP_COAST_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
            break;
        case 50: case 51: case 52: case 53: // goldmine
            g_Map.m_ubPathmapXY[x][y] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG;
            break;
        default: // buildings or decorations
            g_Map.m_ubPathmapXY[x][y] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG;
            break;
    }
}

void mapLoad(tFile *file) {
    g_Map.m_pTileset = TILESETDIR "xxx.bm";
    fileRead(file, (char*)(g_Map.m_pTileset + strlen(TILESETDIR)), 3);

    g_Screen.m_map.m_pTilemap = bitmapCreateFromFile(g_Map.m_pTileset, 0);
    g_Screen.m_map.m_pFogOfWarMask = bitmapCreateFromFile(TILESETDIR "fow.msk", 0);

    for (int x = 0; x < MAP_SIZE; x++) {
        UBYTE *ubColumn = (UBYTE*)(g_Map.m_ubTilemapXY[x]);
        // reads MAP_SIZE bytes into the first half of the column
        fileRead(file, ubColumn, MAP_SIZE);        
        for (int y = MAP_SIZE - 1; y >= 0; --y) {
            UBYTE tileIndex = ubColumn[y]; // map tile index
            g_Map.m_ulVisibleMapXY[x][y] = tileIndexToTileBitmapOffset(tileIndex);
            setFlags(tileIndex, x * TILE_SIZE_FACTOR, y * TILE_SIZE_FACTOR);
        }
    }
}

UBYTE tileBitmapOffsetToTileIndex(ULONG offset) {
    return (offset - (ULONG)g_Screen.m_map.m_pTilemap->Planes[0]) / TILE_FRAME_BYTES;
}

ULONG tileIndexToTileBitmapOffset(UBYTE index) {
    return (ULONG)g_Screen.m_map.m_pTilemap->Planes[0] + TILE_FRAME_BYTES * index;
}

UBYTE mapGetGraphicTileAt(UBYTE x, UBYTE y) {
    return g_Map.m_ubTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR];
}

void mapSetGraphicTileAt(UBYTE x, UBYTE y, UBYTE tileIndex) {
    g_Map.m_ubTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] = tileIndex;
    // g_Map.m_ulVisibleMapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] = tileIndexToTileBitmapOffset(tileIndex);
    setFlags(tileIndex, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}

void mapSetGraphicTileSquare(UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE tileIndex) {
    topLeftX = topLeftX / TILE_SIZE_FACTOR;
    topLeftY = topLeftY / TILE_SIZE_FACTOR;
    size = size / TILE_SIZE_FACTOR;
    UBYTE maxX = topLeftX + size;
    UBYTE maxY = topLeftY + size;
    // ULONG tileOffset = tileIndexToTileBitmapOffset(tileIndex);
    for (UBYTE y = topLeftY; y < maxY; ++y) {
        for (UBYTE x = topLeftX; x < maxX; ++x) {
            g_Map.m_ubTilemapXY[x][y] = tileIndex;
            // g_Map.m_ulVisibleMapXY[x][y] = tileOffset;
            setFlags(tileIndex, x * TILE_SIZE_FACTOR, y * TILE_SIZE_FACTOR);
        }
    }
}

void mapSetBuildingGraphics(UBYTE id, UBYTE extraFlags, UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE tileIndex) {
    topLeftX = topLeftX / TILE_SIZE_FACTOR;
    topLeftY = topLeftY / TILE_SIZE_FACTOR;
    size = size / TILE_SIZE_FACTOR;
    UBYTE maxX = topLeftX + size;
    UBYTE maxY = topLeftY + size;
    UBYTE pathFlag = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | extraFlags;
    for (UBYTE y = topLeftY; y < maxY; ++y) {
        for (UBYTE x = topLeftX; x < maxX; ++x) {
            g_Map.m_ubTilemapXY[x][y] = tileIndex;
            UBYTE pathX = x * TILE_SIZE_FACTOR;
            UBYTE pathY = y * TILE_SIZE_FACTOR;
            g_Map.m_ubPathmapXY[pathX][pathY] = pathFlag;
            g_Map.m_ubPathmapXY[pathX][pathY + 1] = pathFlag;
            g_Map.m_ubPathmapXY[pathX + 1][pathY] = pathFlag;
            g_Map.m_ubPathmapXY[pathX + 1][pathY + 1] = pathFlag;
            g_Map.m_ubUnitCacheXY[pathX][pathY] = id;
            g_Map.m_ubUnitCacheXY[pathX][pathY + 1] = id;
            g_Map.m_ubUnitCacheXY[pathX + 1][pathY] = id;
            g_Map.m_ubUnitCacheXY[pathX + 1][pathY + 1] = id;
            ++tileIndex;
        }
    }
}

void mapIncGraphicTileAt(UBYTE x, UBYTE y) {
    UBYTE newTile = mapGetGraphicTileAt(x, y) + 1;
    g_Map.m_ubTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] += 1;
    // g_Map.m_ulVisibleMapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] += TILE_FRAME_BYTES;
    setFlags(newTile, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}

void mapDecGraphicTileAt(UBYTE x, UBYTE y) {
    UBYTE newTile = mapGetGraphicTileAt(x, y) - 1;
    g_Map.m_ubTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] -= 1;
    // g_Map.m_ulVisibleMapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] -= TILE_FRAME_BYTES;
    setFlags(newTile, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}
