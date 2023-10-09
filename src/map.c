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
            UBYTE tileIndex = ubColumn[y]; // map tile index
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(tileIndex);
            switch (tileIndex) {
                case 0: // grass
                case 1: case 2: case 3: case 4: // bridges
                case 5: // chopped tree
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 6: case 7: case 8: // trees
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_FOREST_FLAG;
                    break;
                case 9: // water
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 10:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 11:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 12:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 13:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 14:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 15:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 16:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 17:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 18:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                case 19:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                case 20:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_WATER_FLAG;
                    break;
                // buildings or decorations
                default:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_UNWALKABLE_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_UNWALKABLE_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_UNWALKABLE_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_UNWALKABLE_FLAG;
                    break;
            }
        }
    }
}
