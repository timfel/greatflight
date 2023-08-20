#include <limits.h>

#include "game.h"
#include "include/map.h"
#include "units.h"
#include "actions.h"
#include "icons.h"
#include "mouse_sprite.h"
#include "selection_sprites.h"
#include <ace/managers/bob.h>
#include <ace/managers/copper.h>
#include <ace/managers/log.h>
#include <ace/managers/mouse.h>
#include <ace/types.h>
#include <ace/managers/viewport/camera.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/custom.h>
#include <ace/utils/extview.h>
#include <ace/utils/file.h>
#include <ace/utils/palette.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>

#include <graphics/sprite.h>

#define BPP 4
#define COLORS (1 << BPP)
#define MAP_WIDTH 320
#define MAP_HEIGHT 160
#define MAP_BUFFER_WIDTH MAP_WIDTH
#define MAP_BUFFER_WIDTH_BYTES (MAP_BUFFER_WIDTH / 8)
#define MAP_BUFFER_HEIGHT MAP_HEIGHT
#define TOP_PANEL_HEIGHT 10
#define BOTTOM_PANEL_HEIGHT 70
#define VISIBLE_TILES_X (MAP_WIDTH >> TILE_SHIFT)
#define VISIBLE_TILES_Y (MAP_HEIGHT >> TILE_SHIFT)
#define CAMERA_MOVE_DELTA 32

static tView *s_pView; // View containing all the viewports

static UBYTE s_ubTopPanelDirty;
static UBYTE s_ubBottomPanelDirty;

static tVPort *s_pVpTopPanel; // Viewport for resources
static tSimpleBufferManager *s_pTopPanelBuffer;
static tBitMap *s_pTopPanelBackground;
static tVPort *s_pVpPanel; // Viewport for panel
static tSimpleBufferManager *s_pPanelBuffer;
static tBitMap *s_pPanelBackground;
static UWORD s_pPanelPalette[COLORS];
static tIcon s_panelUnitIcons[4];
// static tIcon s_panelActionIcons[6];
static tBitMap *s_pIcons;

static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMapBuffer;
static tCameraManager *s_pMainCamera;
static tBitMap *s_pMapBitmap;
static UWORD s_pMapPalette[COLORS];
static ULONG s_ulTilemap[MAP_SIZE][MAP_SIZE];
// static UBYTE s_ubUnitmap[MAP_WIDTH * 2][MAP_HEIGHT * 2];

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
#define LONGEST_MAPNAME "human12.map"

void createViewports() {
    s_pVpTopPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, s_pView,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, TOP_PANEL_HEIGHT,
                             TAG_END);
    s_pVpMain = vPortCreate(0,
                            TAG_VPORT_VIEW, s_pView,
                            TAG_VPORT_BPP, BPP,
                            TAG_VPORT_WIDTH, MAP_WIDTH,
                            TAG_VPORT_HEIGHT, MAP_HEIGHT,
                            TAG_END);
    s_pVpPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, s_pView,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, BOTTOM_PANEL_HEIGHT,
                             TAG_END);
}

ULONG tileIndexToTileBitmapOffset(UBYTE index) {
    return (ULONG)(s_pMapBitmap->Planes[0] + (s_pMapBitmap->BytesPerRow * (index << TILE_SHIFT)));
}

void loadMap(const char* name, UWORD mapbufCoplistStart, UWORD mapColorsCoplistStart) {
    char* mapname = MAPDIR LONGEST_MAPNAME;
    char* palname = IMGDIR "palettes/wood.plt";
    char* imgname = IMGDIR "tilesets/wood.bm";

    snprintf(mapname + strlen(MAPDIR), strlen(LONGEST_MAPNAME) + 1, "%s.map", name);
    tFile *map = fileOpen(mapname, "r");
    if (!map) {
        logWrite("ERROR: Cannot open file %s!\n", mapname);
    }

    // first three bytes are simply name of the palette/terrain
    // fileRead(map, palname + strlen(IMGDIR), 3);
    // strncpy(imgname + strlen(IMGDIR), palname + strlen(IMGDIR), 3);

    logWrite("Loading map: %s %s\n", palname, imgname);
    // create map area
    paletteLoad(palname, s_pMapPalette, COLORS);
    tCopCmd *pCmds = &s_pView->pCopList->pBackBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }

    s_pMapBitmap = bitmapCreateFromFile(imgname, 0);
    s_pMapBuffer = simpleBufferCreate(0,
                                    TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
                                    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                    TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_BUFFER_WIDTH,
                                    TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_BUFFER_HEIGHT,
                                    TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                    TAG_SIMPLEBUFFER_COPLIST_OFFSET, mapbufCoplistStart,
                                    TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
                                    TAG_END);
    s_pMainCamera = cameraCreate(s_pVpMain, 0, 0, MAP_SIZE * TILE_SIZE - MAP_WIDTH, MAP_SIZE * TILE_SIZE - MAP_HEIGHT, 0);

    for (int x = 0; x < MAP_SIZE; x++) {
        UBYTE *ubColumn = (UBYTE*)(s_ulTilemap[x]);
        // reads MAP_SIZE bytes into the first half of the column
        fileRead(map, ubColumn, MAP_SIZE);
        // we store MAP_SIZE words with the direct offsets into the tilemap,
        // so fill from the back the actual offsets
        for (int y = MAP_SIZE - 1; y >= 0; --y) {
            // the fatal error when creating the bitmap above ensures that only
            // the lower 16bits are needed
            s_ulTilemap[x][y] = tileIndexToTileBitmapOffset(ubColumn[y]);
        }
    }
    fileClose(map);

    // TODO: redraw tiles
}

void initBobs(void) {
    // the bobs are not double buffered, since we never undraw
    bobManagerCreate(s_pMapBuffer->pFront, s_pMapBuffer->pBack, MAP_BUFFER_HEIGHT);

    bobInit(&s_TileCursor, TILE_SIZE, TILE_SIZE, 0, s_pMapBitmap->Planes[0], 0, 0, 0);
    bobSetFrame(&s_TileCursor, s_pMapBitmap->Planes[0] + (0x10 * TILE_FRAME_BYTES), 0);

    // s_pUnitManager = unitManagerCreate();
    // unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 7, .ubY = 7});
    // unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 8, .ubY = 7});
    // unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 7, .ubY = 8});
    // unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 8, .ubY = 8});

    // bobReallocateBgBuffers();
}

void loadUi(UWORD topPanelColorsPos, UWORD panelColorsPos, UWORD simplePosTop, UWORD simplePosBottom) {
    // create panel area
    paletteLoad("resources/palettes/hgui.plt", s_pPanelPalette, COLORS);

    tCopCmd *pCmds = &s_pView->pCopList->pBackBfr->pList[topPanelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[topPanelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }

    pCmds = &s_pView->pCopList->pBackBfr->pList[panelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[panelColorsPos];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }

    logWrite("create panel viewport and simple buffer\n");
    s_pTopPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, s_pVpTopPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, simplePosTop,
                                        TAG_END);
    s_pTopPanelBackground = bitmapCreateFromFile("resources/ui/toppanel.bm", 0);
    bitmapLoadFromFile(s_pTopPanelBuffer->pFront, "resources/ui/toppanel.bm", 0, 0);

    s_pPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, s_pVpPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, simplePosBottom,
                                        TAG_END);
    s_pPanelBackground = bitmapCreateFromFile("resources/ui/bottompanel.bm", 0);
    bitmapLoadFromFile(s_pPanelBuffer->pFront, "resources/ui/bottompanel.bm", 0, 0);

    s_pIcons = bitmapCreateFromFile("resources/ui/icons.bm", 0);

    iconInit(&s_panelUnitIcons[0], 32, 26, s_pIcons, 0, s_pPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 18});
    iconInit(&s_panelUnitIcons[1], 32, 26, s_pIcons, 0, s_pPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 18});
    iconInit(&s_panelUnitIcons[2], 32, 26, s_pIcons, 0, s_pPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 44});
    iconInit(&s_panelUnitIcons[3], 32, 26, s_pIcons, 0, s_pPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 44});
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
    s_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, copListLength,
                         TAG_VIEW_WINDOW_HEIGHT, TOP_PANEL_HEIGHT + 1 + MAP_HEIGHT + 1 + BOTTOM_PANEL_HEIGHT,
                         TAG_DONE);

    // setup mouse
    mouseSpriteSetup(s_pView, spritePos);

    // setup selection rectangles
    selectionSpritesSetup(s_pView, selectionPos);

    createViewports();

    // load map file
    loadMap("game2", mapbufCoplistStart, mapColorsCoplistStart);

    initBobs();

    loadUi(topPanelColorsPos, panelColorsPos, simplePosTop, simplePosBottom);

    viewLoad(s_pView);

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
            bobSetFrame(&s_TileCursor, s_pMapBitmap->Planes[0] + (SelectedTile * TILE_FRAME_BYTES), 0);
        } else if (keyCheck(KEY_LBRACKET)) {
            SelectedTile--;
            bobSetFrame(&s_TileCursor, s_pMapBitmap->Planes[0] + (SelectedTile * TILE_FRAME_BYTES), 0);
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
            tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + s_pMainCamera->uPos.ulYX});
            // TODO
            s_ulTilemap[tile.ubX][tile.ubY] = tileIndexToTileBitmapOffset(SelectedTile);
            // tileBufferSetTile(s_pMapBuffer, tile.ubX, tile.ubY, SelectedTile);
            // tileBufferQueueProcess(s_pMapBuffer);
            // bobDiscardUndraw();
        }
        return;
    }

    if (keyCheck(KEY_ESCAPE)) {
        gameExit();
    } else if (keyCheck(KEY_C)) {
        copDumpBfr(s_pView->pCopList->pBackBfr);
    } else if (keyCheck(KEY_E)) {
        s_Mode = edit;
    } else if (keyCheck(KEY_UP)) {
        cameraMoveBy(s_pMainCamera, 0, -CAMERA_MOVE_DELTA);
        s_pMapBuffer->pCamera->uPos.uwY = s_pMainCamera->uPos.uwY % TILE_SIZE;
    } else if (keyCheck(KEY_DOWN)) {
        cameraMoveBy(s_pMainCamera, 0, CAMERA_MOVE_DELTA);
        s_pMapBuffer->pCamera->uPos.uwY = s_pMainCamera->uPos.uwY % TILE_SIZE;
    } else if (keyCheck(KEY_LEFT)) {
        cameraMoveBy(s_pMainCamera, -CAMERA_MOVE_DELTA, 0);
        s_pMapBuffer->pCamera->uPos.uwX = s_pMainCamera->uPos.uwX % TILE_SIZE;
    } else if (keyCheck(KEY_RIGHT)) {
        cameraMoveBy(s_pMainCamera, CAMERA_MOVE_DELTA, 0);
        s_pMapBuffer->pCamera->uPos.uwX = s_pMainCamera->uPos.uwX % TILE_SIZE;
    }
    if (!mousePos.uwY) {
        cameraMoveBy(s_pMainCamera, 0, -CAMERA_MOVE_DELTA);
        s_pMapBuffer->pCamera->uPos.uwY = s_pMainCamera->uPos.uwY % TILE_SIZE;
    } else if (mousePos.uwY >= MAP_HEIGHT + BOTTOM_PANEL_HEIGHT - 1) {
        cameraMoveBy(s_pMainCamera, 0, CAMERA_MOVE_DELTA);
        s_pMapBuffer->pCamera->uPos.uwY = s_pMainCamera->uPos.uwY % TILE_SIZE;
    }
    if (!mousePos.uwX) {
        cameraMoveBy(s_pMainCamera, -CAMERA_MOVE_DELTA, 0);
        s_pMapBuffer->pCamera->uPos.uwX = s_pMainCamera->uPos.uwX % TILE_SIZE;
    } else if (mousePos.uwX > MAP_WIDTH - 1) {
        cameraMoveBy(s_pMainCamera, CAMERA_MOVE_DELTA, 0);
        s_pMapBuffer->pCamera->uPos.uwX = s_pMainCamera->uPos.uwX % TILE_SIZE;
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
        tUbCoordYX tile1 = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + s_pMainCamera->uPos.ulYX});
        tUbCoordYX tile2 = screenPosToTile((tUwCoordYX){.ulYX = lmbDown.ulYX + s_pMainCamera->uPos.ulYX});
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
                                s_ubBottomPanelDirty = 1;
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
                            s_ubBottomPanelDirty = 1;
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
                s_ubBottomPanelDirty = 1;
            }
        }
        lmbDown.ulYX = 0;
        selectionRectangleUpdate(-1, -1, -1, -1);
    } else if (s_pSelectedUnit[0] && mouseCheck(MOUSE_PORT_1, MOUSE_RMB)) {
        tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + s_pMainCamera->uPos.ulYX});
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
    if (s_ubTopPanelDirty) {
        // TODO: only store and redraw the dirty part
        s_ubTopPanelDirty = 0;
        // the panel is never actually swapped, the backbuffer is just the plain panel for redraw
        blitWait(); // Don't modify registers when other blit is in progress
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xffff;
        g_pCustom->bltalwm = 0xffff;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = 0;
        g_pCustom->bltapt = s_pTopPanelBackground->Planes[0];
        g_pCustom->bltdpt = s_pTopPanelBuffer->pFront->Planes[0];
        g_pCustom->bltsize = ((TOP_PANEL_HEIGHT * BPP) << 6) | (MAP_WIDTH >> 4);
    }

    if (s_ubBottomPanelDirty) {
        // TODO: only store and redraw the dirty part?
        s_ubBottomPanelDirty = 0;
        // the panel is never actually swapped, the backbuffer is just the plain panel for redraw
        blitWait(); // Don't modify registers when other blit is in progress
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xffff;
        g_pCustom->bltalwm = 0xffff;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = 0;
        g_pCustom->bltapt = s_pPanelBackground->Planes[0];
        g_pCustom->bltdpt = s_pPanelBuffer->pFront->Planes[0];
        g_pCustom->bltsize = ((BOTTOM_PANEL_HEIGHT * BPP) << 6) | (MAP_WIDTH >> 4);

        Unit *unit;
        for(UBYTE idx = 0; idx < NUM_SELECTION && (unit = s_pSelectedUnit[idx]); ++idx) {
            tIcon *icon = &s_panelUnitIcons[idx];
            iconSetSource(icon, s_pIcons, UnitTypes[unit->type].iconIdx);
            iconDraw(icon);
        }
    }
}

void drawSelectionRectangles(void) {
    for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
        Unit *unit = s_pSelectedUnit[idx];
        if (unit) {
            WORD bobPosOnScreenX = s_pSelectedUnit[idx]->bob.sPos.uwX - s_pMainCamera->uPos.uwX;
            if (bobPosOnScreenX >= -8) {
                WORD bobPosOnScreenY = s_pSelectedUnit[idx]->bob.sPos.uwY - s_pMainCamera->uPos.uwY + TOP_PANEL_HEIGHT;
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
	WORD wDstModulo = MAP_BUFFER_WIDTH_BYTES - TILE_SIZE_BYTES; // same as bitmapGetByteWidth(s_pMapBuffer->pBack) - TILE_SIZE_BYTES;
    WORD wSrcModulo = 0;
	UWORD uwBlitWords = TILE_SIZE_WORDS;
	UWORD uwHeight = TILE_SIZE * BPP;
	UWORD uwBltCon0 = USEA|USED|MINTERM_A;
	UWORD uwBltsize = (uwHeight << 6) | uwBlitWords;

    // Figure out which tiles to actually draw, depending on the
    // current camera position
    UBYTE ubStartX = /*MAX(0, (*/s_pMainCamera->uPos.uwX >> TILE_SHIFT/*))*/;
	UBYTE ubStartY = /*MAX(0, (*/s_pMainCamera->uPos.uwY >> TILE_SHIFT/*))*/;
	UBYTE ubEndX = ubStartX + (MAP_WIDTH >> TILE_SHIFT);

    // Get pointer to start of drawing area
    PLANEPTR pDstPlane = s_pMapBuffer->pBack->Planes[0];

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
        ULONG *pTileBitmapOffset = &(s_ulTilemap[x][ubStartY]);
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

    // redraw units and missiles
    bobBegin(s_pMapBuffer->pBack);

    // process all units
    // tUbCoordYX tileTopLeft = screenPosToTile(s_pMainCamera->uPos);
    // unitManagerProcessUnits(
    //     s_pUnitManager,
    //     (UBYTE **)s_ubUnitmap,
    //     tileTopLeft,
    //     (tUbCoordYX){.ubX = tileTopLeft.ubX + VISIBLE_TILES_X, .ubY = tileTopLeft.ubY + VISIBLE_TILES_Y}
    // );

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
    viewProcessManagers(s_pView);
    copSwapBuffers();
    vPortWaitUntilEnd(s_pVpPanel);

    // update sprites during vblank
    mouseSpriteUpdate(mouseX, mouseY);
    drawSelectionRectangles();

    GameCycle++;
}

void gameGsDestroy(void) {
    systemUse();

    // This will also destroy all associated viewports and viewport managers
    viewDestroy(s_pView);

    unitManagerDestroy(s_pUnitManager);

    bobManagerDestroy();
    bitmapDestroy(s_pMapBitmap);
}
