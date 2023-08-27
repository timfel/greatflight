#include <limits.h>

#include "game.h"
#include "include/map.h"
#include "include/player.h"
#include "include/units.h"
#include "actions.h"
#include "mouse_sprite.h"
#include "selection_sprites.h"

#include <graphics/sprite.h>

#define VISIBLE_TILES_X (MAP_WIDTH >> TILE_SHIFT)
#define VISIBLE_TILES_Y (MAP_HEIGHT >> TILE_SHIFT)
#define CAMERA_MOVE_DELTA 32

struct Screen g_Screen;
static tBob s_TileCursor;
static tUnitManager *s_pUnitManager;

enum mode {
    game,
    edit
};
static UWORD GameCycle = 0;
static UBYTE s_Mode = game;

// editor statics
static UBYTE SelectedTile = 0x10;

// game statics
static Unit *s_pSelectedUnit[NUM_SELECTION];

#define IMGDIR "resources/"
#define PALDIR IMGDIR "palettes/"
#define TILESETDIR IMGDIR "tilesets/"
#define LONGEST_MAPNAME "human12.map"

void createViewports() {
    g_Screen.m_panels.m_pTopPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, g_Screen.m_pView,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, TOP_PANEL_HEIGHT,
                             TAG_END);
    g_Screen.m_map.m_pVPort = vPortCreate(0,
                            TAG_VPORT_VIEW, g_Screen.m_pView,
                            TAG_VPORT_BPP, BPP,
                            TAG_VPORT_WIDTH, MAP_WIDTH,
                            TAG_VPORT_HEIGHT, MAP_HEIGHT,
                            TAG_END);
    g_Screen.m_panels.m_pMainPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, g_Screen.m_pView,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, BOTTOM_PANEL_HEIGHT,
                             TAG_END);
}

ULONG tileIndexToTileBitmapOffset(UBYTE index) {
    return (ULONG)(g_Screen.m_map.m_pTilemap->Planes[0] + (g_Screen.m_map.m_pTilemap->BytesPerRow * (index << TILE_SHIFT)));
}

void loadMap(const char* name, UWORD mapbufCoplistStart, UWORD mapColorsCoplistStart) {
    char* mapname = MAPDIR LONGEST_MAPNAME;
    char* palname = PALDIR "for.plt";
    g_Map.m_pTileset = TILESETDIR "for.bm";

    snprintf(mapname + strlen(MAPDIR), strlen(LONGEST_MAPNAME) + 1, "%s.map", name);
    tFile *map = fileOpen(mapname, "r");
    if (!map) {
        logWrite("ERROR: Cannot open file %s!\n", mapname);
    }

    // first three bytes are simply name of the palette/terrain
    fileRead(map, palname + strlen(PALDIR), 3);
    strncpy((char*)(g_Map.m_pTileset + strlen(TILESETDIR)), palname + strlen(PALDIR), 3);

    logWrite("Loading map: %s %s\n", palname, g_Map.m_pTileset);
    // create map area
    paletteLoad(palname, g_Screen.m_map.m_pPalette, COLORS);
    tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }

    g_Screen.m_map.m_pTilemap = bitmapCreateFromFile(g_Map.m_pTileset, 0);
    g_Screen.m_map.m_pBuffer = simpleBufferCreate(0,
                                    TAG_SIMPLEBUFFER_VPORT, g_Screen.m_map.m_pVPort,
                                    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                    TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_BUFFER_WIDTH,
                                    TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_BUFFER_HEIGHT,
                                    TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                    TAG_SIMPLEBUFFER_COPLIST_OFFSET, mapbufCoplistStart,
                                    TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
                                    TAG_END);
    g_Screen.m_map.m_pCamera = cameraCreate(g_Screen.m_map.m_pVPort, 0, 0, MAP_SIZE * TILE_SIZE - MAP_WIDTH, MAP_SIZE * TILE_SIZE - MAP_HEIGHT, 0);

    for (int x = 0; x < MAP_SIZE; x++) {
        UBYTE *ubColumn = (UBYTE*)(g_Map.m_ulTilemapXY[x]);
        // reads MAP_SIZE bytes into the first half of the column
        fileRead(map, ubColumn, MAP_SIZE);
        // we store MAP_SIZE words with the direct offsets into the tilemap,
        // so fill from the back the actual offsets
        for (int y = MAP_SIZE - 1; y >= 0; --y) {
            g_Map.m_ulTilemapXY[x][y] = tileIndexToTileBitmapOffset(ubColumn[y]);
        }
    }

    loadPlayerInfo(map);

    loadUnits(s_pUnitManager, map);

    fileClose(map);
}

void initBobs(void) {
    bobManagerCreate(g_Screen.m_map.m_pBuffer->pFront, g_Screen.m_map.m_pBuffer->pBack, MAP_BUFFER_HEIGHT);
    bobInit(&s_TileCursor, TILE_SIZE, TILE_SIZE, 0, g_Screen.m_map.m_pTilemap->Planes[0], 0, 0, 0);
    bobSetFrame(&s_TileCursor, g_Screen.m_map.m_pTilemap->Planes[0] + (0x10 * TILE_FRAME_BYTES), 0);
}

void loadUi(UWORD topPanelColorsPos, UWORD panelColorsPos, UWORD simplePosTop, UWORD simplePosBottom) {
    // create panel area
    paletteLoad("resources/palettes/hgui.plt", g_Screen.m_panels.m_pPalette, COLORS);

    tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[topPanelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[topPanelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }

    pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[panelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[panelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }

    logWrite("create panel viewport and simple buffer\n");
    g_Screen.m_panels.m_pTopPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, g_Screen.m_panels.m_pTopPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, simplePosTop,
                                        TAG_END);
    g_Screen.m_panels.s_pTopPanelBackground = bitmapCreateFromFile("resources/ui/toppanel.bm", 0);
    bitmapLoadFromFile(g_Screen.m_panels.m_pTopPanelBuffer->pFront, "resources/ui/toppanel.bm", 0, 0);

    g_Screen.m_panels.m_pMainPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, g_Screen.m_panels.m_pMainPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, simplePosBottom,
                                        TAG_END);
    g_Screen.m_panels.m_pMainPanelBackground = bitmapCreateFromFile("resources/ui/bottompanel.bm", 0);
    bitmapLoadFromFile(g_Screen.m_panels.m_pMainPanelBuffer->pFront, "resources/ui/bottompanel.bm", 0, 0);

    g_Screen.m_pIcons = bitmapCreateFromFile("resources/ui/icons.bm", 0);

    iconInit(&g_Screen.m_pUnitIcons[0], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 18});
    iconInit(&g_Screen.m_pUnitIcons[1], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 18});
    iconInit(&g_Screen.m_pUnitIcons[2], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 44});
    iconInit(&g_Screen.m_pUnitIcons[3], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 44});
}

void gameGsCreate(void) {
    viewLoad(0);

    // Calculate copperlist size
    UWORD spritePos = 0;
    UWORD selectionPos = spritePos + mouseSpriteGetRawCopplistInstructionCountLength();
    UWORD simplePosTop = selectionPos + selectionSpritesGetRawCopplistInstructionCountLength();
    UWORD topPanelColorsPos = simplePosTop + simpleBufferGetRawCopperlistInstructionCount(BPP);
    UWORD mapbufCoplistStart = topPanelColorsPos + COLORS;
    UWORD mapColorsCoplistStart = mapbufCoplistStart + simpleBufferGetRawCopperlistInstructionCount(BPP);
    UWORD simplePosBottom = mapColorsCoplistStart + COLORS;
    UWORD panelColorsPos = simplePosBottom + simpleBufferGetRawCopperlistInstructionCount(BPP);
    UWORD copListLength = panelColorsPos + COLORS;

    // Create the game view
    g_Screen.m_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, copListLength,
                         TAG_VIEW_WINDOW_HEIGHT, TOP_PANEL_HEIGHT + 1 + MAP_HEIGHT + 1 + BOTTOM_PANEL_HEIGHT,
                         TAG_DONE);

    // setup mouse
    mouseSpriteSetup(g_Screen.m_pView, spritePos);

    // setup selection rectangles
    selectionSpritesSetup(g_Screen.m_pView, selectionPos);

    createViewports();

    s_pUnitManager = unitManagerCreate();

    // load map file
    loadMap(g_Map.m_pName, mapbufCoplistStart, mapColorsCoplistStart);

    initBobs();

    loadUi(topPanelColorsPos, panelColorsPos, simplePosTop, simplePosBottom);

    viewLoad(g_Screen.m_pView);

    systemSetDmaMask(DMAF_SPRITE, 1);

    systemUnuse();
}

static inline tUbCoordYX screenPosToTile(tUwCoordYX pos) {
    return (tUbCoordYX){.ubX = pos.uwX >> TILE_SHIFT, .ubY = pos.uwY >> TILE_SHIFT};
}

void handleInput(UWORD mouseX, UWORD mouseY) {
    static tUwCoordYX lmbDown = {.ulYX = 0};
    
    tUwCoordYX mousePos = {.uwX = mouseX / TILE_SIZE * TILE_SIZE, .uwY = mouseY / TILE_SIZE * TILE_SIZE};

    if (s_Mode == edit) {
        s_TileCursor.sPos.ulYX = mousePos.ulYX;
        if (keyCheck(KEY_RBRACKET)) {
            SelectedTile++;
            bobSetFrame(&s_TileCursor, g_Screen.m_map.m_pTilemap->Planes[0] + (SelectedTile * TILE_FRAME_BYTES), 0);
        } else if (keyCheck(KEY_LBRACKET)) {
            SelectedTile--;
            bobSetFrame(&s_TileCursor, g_Screen.m_map.m_pTilemap->Planes[0] + (SelectedTile * TILE_FRAME_BYTES), 0);
        } else if (keyCheck(KEY_RETURN)) {
            const char* mapname = MAPDIR "game.map";
            tFile *map = fileOpen(mapname, "w");
            if (!map) {
                logWrite("ERROR: Cannot open file %s!\n", mapname);
            } else {
                fileWrite(map, "for", 3);
                for (int x = 0; x < MAP_SIZE; x++) {
                    // TODO: fix file writing
                    // fileWrite(map, s_ulTilemap[x], MAP_SIZE);
                }
            }
        } else if (keyCheck(KEY_G)) {
            s_Mode = game;
        } else if (mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + g_Screen.m_map.m_pCamera->uPos.ulYX});
            // TODO
            g_Map.m_ulTilemapXY[tile.ubX][tile.ubY] = tileIndexToTileBitmapOffset(SelectedTile);
            // tileBufferSetTile(g_Screen.m_map.m_pBuffer, tile.ubX, tile.ubY, SelectedTile);
            // tileBufferQueueProcess(g_Screen.m_map.m_pBuffer);
            // bobDiscardUndraw();
        }
        return;
    }

    if (keyCheck(KEY_ESCAPE)) {
        gameExit();
    } else if (keyCheck(KEY_C)) {
        copDumpBfr(g_Screen.m_pView->pCopList->pBackBfr);
    } else if (keyCheck(KEY_E)) {
        s_Mode = edit;
    } else if (keyCheck(KEY_UP)) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, 0, -CAMERA_MOVE_DELTA);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
    } else if (keyCheck(KEY_DOWN)) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, 0, CAMERA_MOVE_DELTA);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
    } else if (keyCheck(KEY_LEFT)) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, -CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    } else if (keyCheck(KEY_RIGHT)) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    }
    if (!mousePos.uwY) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, 0, -CAMERA_MOVE_DELTA);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
    } else if (mousePos.uwY >= MAP_HEIGHT + BOTTOM_PANEL_HEIGHT - 1) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, 0, CAMERA_MOVE_DELTA);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
    }
    if (!mousePos.uwX) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, -CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    } else if (mousePos.uwX > MAP_WIDTH - 1) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    }

    if (mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
        if (lmbDown.uwY) {
            if (lmbDown.uwY - mousePos.uwY) {
                selectionRectangleUpdate(lmbDown.uwX, mousePos.uwX, lmbDown.uwY + TOP_PANEL_HEIGHT, mousePos.uwY + TOP_PANEL_HEIGHT);
            }
        } else {
            lmbDown = mousePos;
            selectionRectangleUpdate(-1, -1, -1, -1);
        }
    } else if (lmbDown.ulYX) {
        tUbCoordYX tile1 = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + g_Screen.m_map.m_pCamera->uPos.ulYX});
        tUbCoordYX tile2 = screenPosToTile((tUwCoordYX){.ulYX = lmbDown.ulYX + g_Screen.m_map.m_pCamera->uPos.ulYX});
        UBYTE tmp = tile1.ubX - tile2.ubX;
        UBYTE operand = tmp & (tmp >> (sizeof(int) * CHAR_BIT - 1));
        UBYTE x1 = tile2.ubX + operand; // min(x, y)
        UBYTE x2 = tile1.ubX - operand; // max(x, y)
        tmp = tile1.ubY - tile2.ubY;
        operand = tmp & (tmp >> (sizeof(int) * CHAR_BIT - 1));
        UBYTE y1 = tile2.ubY + operand; // min(x, y)
        UBYTE y2 = tile1.ubY - operand; // max(x, y)
        tmp = 0;
        while (y1 <= y2) {
            UBYTE xcur = x1;
            while (xcur <= x2) {
                tUbCoordYX tile = (tUbCoordYX){.ubX = xcur, .ubY = y1};
                Unit *unit = unitManagerUnitAt(s_pUnitManager, tile);
                if (unit) {
                    if (keyCheck(KEY_LSHIFT) || keyCheck(KEY_RSHIFT) || tmp) {
                        for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
                            Unit *sel = s_pSelectedUnit[idx];
                            if (!sel) {
                                s_pSelectedUnit[idx] = unit;
                                g_Screen.m_ubBottomPanelDirty = 1;
                                break;
                            }
                            if (sel == unit) {
                                break;
                            }
                        }
                    } else {
                        s_pSelectedUnit[0] = unit;
                        for(UBYTE idx = 1; idx < NUM_SELECTION; ++idx) {
                            s_pSelectedUnit[idx] = NULL;
                            g_Screen.m_ubBottomPanelDirty = 1;
                        }
                    }
                    ++tmp;
                }
                ++xcur;
            }
            ++y1;
        }
        if (!tmp) {
            for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
                s_pSelectedUnit[idx] = NULL;
                g_Screen.m_ubBottomPanelDirty = 1;
            }
        }
        lmbDown.ulYX = 0;
        selectionRectangleUpdate(-1, -1, -1, -1);
    } else if (s_pSelectedUnit[0] && mouseCheck(MOUSE_PORT_1, MOUSE_RMB)) {
        tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + g_Screen.m_map.m_pCamera->uPos.ulYX});
        for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
            Unit *unit = s_pSelectedUnit[idx];
            if (unit) {
                actionMoveTo(unit, tile);
            } else {
                break;
            }
        }
    }
}

void drawPanel(void) {
    if (g_Screen.m_ubTopPanelDirty) {
        // TODO: only store and redraw the dirty part
        g_Screen.m_ubTopPanelDirty = 0;
        // the panel is never actually swapped, the backbuffer is just the plain panel for redraw
        blitWait(); // Don't modify registers when other blit is in progress
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xffff;
        g_pCustom->bltalwm = 0xffff;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = 0;
        g_pCustom->bltapt = g_Screen.m_panels.s_pTopPanelBackground->Planes[0];
        g_pCustom->bltdpt = g_Screen.m_panels.m_pTopPanelBuffer->pFront->Planes[0];
        g_pCustom->bltsize = ((TOP_PANEL_HEIGHT * BPP) << 6) | (MAP_WIDTH >> 4);
    }

    if (g_Screen.m_ubBottomPanelDirty) {
        // TODO: only store and redraw the dirty part?
        g_Screen.m_ubBottomPanelDirty = 0;
        // the panel is never actually swapped, the backbuffer is just the plain panel for redraw
        blitWait(); // Don't modify registers when other blit is in progress
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xffff;
        g_pCustom->bltalwm = 0xffff;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = 0;
        g_pCustom->bltapt = g_Screen.m_panels.m_pMainPanelBackground->Planes[0];
        g_pCustom->bltdpt = g_Screen.m_panels.m_pMainPanelBuffer->pFront->Planes[0];
        g_pCustom->bltsize = ((BOTTOM_PANEL_HEIGHT * BPP) << 6) | (MAP_WIDTH >> 4);

        Unit *unit;
        for(UBYTE idx = 0; idx < NUM_SELECTION && (unit = s_pSelectedUnit[idx]); ++idx) {
            tIcon *icon = &g_Screen.m_pUnitIcons[idx];
            iconSetSource(icon, g_Screen.m_pIcons, UnitTypes[unit->type].iconIdx);
            iconDraw(icon);
        }
    }
}

void drawSelectionRectangles(void) {
    for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
        Unit *unit = s_pSelectedUnit[idx];
        if (unit) {
            WORD bobPosOnScreenX = s_pSelectedUnit[idx]->bob.sPos.uwX - g_Screen.m_map.m_pCamera->uPos.uwX;
            if (bobPosOnScreenX >= -8) {
                WORD bobPosOnScreenY = s_pSelectedUnit[idx]->bob.sPos.uwY - g_Screen.m_map.m_pCamera->uPos.uwY + TOP_PANEL_HEIGHT;
                if (bobPosOnScreenX >= -8) {
                    selectionSpritesUpdate(idx, bobPosOnScreenX + 8, bobPosOnScreenY + 8);
                    continue;
                }
            }
        }
        selectionSpritesUpdate(idx, -1, -1);
    }
}

FN_HOTSPOT
void drawAllTiles(void) {
    // setup tile drawing, all of these should be compiled to immediate operations, they
    // only use constants
	WORD wDstModulo = MAP_BUFFER_WIDTH_BYTES - TILE_SIZE_BYTES; // same as bitmapGetByteWidth(g_Screen.m_map.m_pBuffer->pBack) - TILE_SIZE_BYTES;
    WORD wSrcModulo = 0;
	UWORD uwBlitWords = TILE_SIZE_WORDS;
	UWORD uwHeight = TILE_SIZE * BPP;
	UWORD uwBltCon0 = USEA|USED|MINTERM_A;
	UWORD uwBltsize = (uwHeight << 6) | uwBlitWords;

    // Figure out which tiles to actually draw, depending on the
    // current camera position
    UBYTE ubStartX = /*MAX(0, (*/g_Screen.m_map.m_pCamera->uPos.uwX >> TILE_SHIFT/*))*/;
	UBYTE ubStartY = /*MAX(0, (*/g_Screen.m_map.m_pCamera->uPos.uwY >> TILE_SHIFT/*))*/;
	UBYTE ubEndX = ubStartX + (MAP_WIDTH >> TILE_SHIFT);

    // Get pointer to start of drawing area
    PLANEPTR pDstPlane = g_Screen.m_map.m_pBuffer->pBack->Planes[0];

    // setup blitter registers that won't change
    systemSetDmaBit(DMAB_BLITHOG, 1);
    blitWait();
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = wSrcModulo;
	g_pCustom->bltdmod = wDstModulo;

    // draw as fast as we can
    for (UBYTE x = ubStartX; x < ubEndX; x++) {
        // manually unrolled loop to draw (MAP_BUFFER_HEIGHT / TILE_SIZE) tiles
        ULONG *pTileBitmapOffset = &(g_Map.m_ulTilemapXY[x][ubStartY]);
        blitWait();
        g_pCustom->bltdpt = pDstPlane;
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        blitWait();
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        blitWait();
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        blitWait();
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        blitWait();
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        pDstPlane += (TILE_SIZE >> 3);
    }
    systemSetDmaBit(DMAB_BLITHOG, 0);
}

void gameGsLoop(void) {
    // redraw map
    drawAllTiles();
    // vPortWaitUntilEnd(g_Screen.m_panels.m_pMainPanel);

    // redraw units and missiles
    bobBegin(g_Screen.m_map.m_pBuffer->pBack);

    // process all units
    tUbCoordYX tileTopLeft = screenPosToTile(g_Screen.m_map.m_pCamera->uPos);
    unitManagerProcessUnits(
        s_pUnitManager,
        (UBYTE **)g_Map.m_ubPathmapXY,
        (tUbCoordYX){.ubX = tileTopLeft.ubX * 2, .ubY = tileTopLeft.ubY * 2},
        (tUbCoordYX){.ubX = (tileTopLeft.ubX + VISIBLE_TILES_X) * 2, .ubY = (tileTopLeft.ubY + VISIBLE_TILES_Y) * 2}
    );

    UWORD mouseX = mouseGetX(MOUSE_PORT_1);
    UWORD mouseY = mouseGetY(MOUSE_PORT_1);

    // do other actions (AI, sounds)
    UBYTE renderCycle = GameCycle & 0b1111;
    switch (renderCycle) {
        // handle input once per 8 frames
        case 0b0000:
        case 0b1000:
            handleInput(mouseX, mouseY);
            break;
        // handle income once per 16 frames
        case 0b0001:
            // redrawIncome();
            break;
        default:
            break;
    }

    if (s_Mode == edit) {
        bobPush(&s_TileCursor);
    }

    bobEnd();

    // redraw panel
    drawPanel();

    // finish frame
    viewProcessManagers(g_Screen.m_pView);
    copSwapBuffers();
    vPortWaitUntilEnd(g_Screen.m_panels.m_pMainPanel);

    // update sprites during vblank
    mouseSpriteUpdate(mouseX, mouseY);
    drawSelectionRectangles();

    GameCycle++;
}

void gameGsDestroy(void) {
    systemUse();

    // This will also destroy all associated viewports and viewport managers
    viewDestroy(g_Screen.m_pView);

    unitManagerDestroy(s_pUnitManager);

    bobManagerDestroy();
    bitmapDestroy(g_Screen.m_map.m_pTilemap);
}
