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
            UBYTE tileIndex = ubColumn[y];
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(tileIndex);
            switch (tileIndex) {
                // ground
                case 0: case 1: case 2: case 3:
                case 4: case 5: case 6: case 7:
                case 8: case 9: case 10: case 11:
                case 12: case 13: case 14: case 15:
                case 16: case 17: case 18: case 19:
                case 20: case 21: case 22: case 23:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_GROUND_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_GROUND_FLAG;
                    break;
                // coast
                case 40: case 41:
                case 44: case 45: case 47:
                case 48: case 49: case 50:
                case 52: case 54:
                case 57: case 59:
                case 60: case 62: case 63:
                case 66: case 67:
                case 69: case 70: case 71:
                case 72:
                case 77:
                case 82:
                case 87:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_COAST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_COAST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_COAST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_COAST_FLAG;
                    break;
                // forest
                case 24: case 25: case 26: case 27:
                case 28: case 29: case 30: case 31:
                case 32: case 33: case 34: case 35:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_FOREST_FLAG;
                    g_Map.m_ubPathmapXY[x * 2][y * 2 + 1] = MAP_FOREST_FLAG;
                    break;
                // water
                case 36: case 37: case 38: case 39:
                    g_Map.m_ubPathmapXY[x * 2][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2] = MAP_WATER_FLAG;
                    g_Map.m_ubPathmapXY[x * 2 + 1][y * 2 + 1] = MAP_WATER_FLAG;
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
