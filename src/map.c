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
        // buildings or decorations
        default:
            g_Map.m_ubPathmapXY[x][y] = MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y] = MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x + 1][y + 1] = MAP_UNWALKABLE_FLAG;
            g_Map.m_ubPathmapXY[x][y + 1] = MAP_UNWALKABLE_FLAG;
            break;
    }
}

void mapLoad(tFile *file) {
    g_Map.m_pTileset = TILESETDIR "xxx.bm";
    fileRead(file, (char*)(g_Map.m_pTileset + strlen(TILESETDIR)), 3);

    g_Screen.m_map.m_pTilemap = bitmapCreateFromFile(g_Map.m_pTileset, 0);

    for (int x = 0; x < MAP_SIZE; x++) {
        UBYTE *ubColumn = (UBYTE*)(g_Map.m_ulTilemapXY[x]);
        // reads MAP_SIZE bytes into the first half of the column
        fileRead(file, ubColumn, MAP_SIZE);
        // we store MAP_SIZE words with the direct offsets into the tilemap,
        // so fill from the back the actual offsets
        for (int y = MAP_SIZE - 1; y >= 0; --y) {
            UBYTE tileIndex = ubColumn[y]; // map tile index
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(tileIndex);
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
    return tileBitmapOffsetToTileIndex(g_Map.m_ulTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR]);
}

void mapSetGraphicTileAt(UBYTE x, UBYTE y, UBYTE tileIndex) {
    g_Map.m_ulTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] = tileIndexToTileBitmapOffset(tileIndex);
    setFlags(tileIndex, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}

void mapSetGraphicTileSquare(UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE tileIndex) {
    topLeftX = topLeftX / TILE_SIZE_FACTOR;
    topLeftY = topLeftY / TILE_SIZE_FACTOR;
    size = size / TILE_SIZE_FACTOR;
    UBYTE maxX = topLeftX + size;
    UBYTE maxY = topLeftY + size;
    ULONG tileOffset = tileIndexToTileBitmapOffset(tileIndex);
    for (UBYTE y = topLeftY; y < maxY; ++y) {
        for (UBYTE x = topLeftX; x < maxX; ++x) {
            g_Map.m_ulTilemapXY[x][y] = tileOffset;
            setFlags(tileIndex, x * TILE_SIZE_FACTOR, y * TILE_SIZE_FACTOR);
        }
    }
}

void mapSetGraphicTileRangeSquare(UBYTE topLeftX, UBYTE topLeftY, UBYTE size, UBYTE tileIndex) {
    topLeftX = topLeftX / TILE_SIZE_FACTOR;
    topLeftY = topLeftY / TILE_SIZE_FACTOR;
    size = size / TILE_SIZE_FACTOR;
    UBYTE maxX = topLeftX + size;
    UBYTE maxY = topLeftY + size;
    for (UBYTE y = topLeftY; y < maxY; ++y) {
        for (UBYTE x = topLeftX; x < maxX; ++x) {
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(tileIndex);
            setFlags(tileIndex, x * TILE_SIZE_FACTOR, y * TILE_SIZE_FACTOR);
            ++tileIndex;
        }
    }
}

void mapIncGraphicTileAt(UBYTE x, UBYTE y) {
    UBYTE newTile = mapGetGraphicTileAt(x, y) + 1;
    g_Map.m_ulTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] += TILE_FRAME_BYTES;
    setFlags(newTile, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}

void mapDecGraphicTileAt(UBYTE x, UBYTE y) {
    UBYTE newTile = mapGetGraphicTileAt(x, y) - 1;
    g_Map.m_ulTilemapXY[x / TILE_SIZE_FACTOR][y / TILE_SIZE_FACTOR] -= TILE_FRAME_BYTES;
    setFlags(newTile, x / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, y / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR);
}
