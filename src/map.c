#include "include/map.h"
#include "include/resources.h"
#include "game.h"
#include "string.h"

struct Map g_Map;

void mapLoad(tFile *file, void(*loadTileBitmap)()) {
    g_Map.m_pTileset = TILESETDIR "xxx.bm";
    fileRead(file, (char*)(g_Map.m_pTileset + strlen(TILESETDIR)), 3);

    loadTileBitmap();

    for (int x = 0; x < MAP_SIZE; x++) {
        UBYTE *ubColumn = (UBYTE*)(g_Map.m_ulTilemapXY[x]);
        // reads MAP_SIZE bytes into the first half of the column
        fileRead(file, ubColumn, MAP_SIZE);
        // we store MAP_SIZE words with the direct offsets into the tilemap,
        // so fill from the back the actual offsets
        for (int y = MAP_SIZE - 1; y >= 0; --y) {
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(ubColumn[y]);
        }
    }
}
