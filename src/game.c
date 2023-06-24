#include <stdint.h>
#include <limits.h>

#include "game.h"
#include "bob_new.h"
#include "map.h"
#include "units.h"
#include "actions.h"
#include "icons.h"
#include "mouse_sprite.h"
#include "selection_sprites.h"
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
#include <ace/managers/viewport/scrollbuffer.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/blit.h>

#include <graphics/sprite.h>

#define BPP 5
#define COLORS (1 << BPP)

static tView *s_pView; // View containing all the viewports

static tVPort *s_pVpTopPanel; // Viewport for resources
static tSimpleBufferManager *s_pTopPanelBuffer;
static tBitMap *s_pTopPanelBackground;
static tVPort *s_pVpPanel; // Viewport for panel
static tSimpleBufferManager *s_pPanelBuffer;
static tBitMap *s_pPanelBackground;
static uint16_t s_pPanelPalette[COLORS];
static tIcon s_panelUnitIcons[4];
static tIcon s_panelActionIcons[6];
static tBitMap *s_pIcons;

static tVPort *s_pVpMain; // Viewport for playfield
static tTileBufferManager *s_pMapBuffer;
static tCameraManager *s_pMainCamera;
static tBitMap *s_pMapBitmap;
static uint16_t s_pMapPalette[COLORS];

static tBobNew s_TileCursor;

static tUnitManager *s_pUnitManager;

enum mode {
    game,
    edit
};
static uint16_t GameCycle = 0;
static UBYTE s_Mode = game;

// editor statics
static UBYTE SelectedTile = 0x10;

// game statics
static Unit *s_pSelectedUnit[NUM_SELECTION];

#define IMGDIR "resources/imgs/"
#define MAPDIR "resources/maps/"
#define LONGEST_MAPNAME "human12.map"

#define MAP_WIDTH 320
#define MAP_HEIGHT 160
#define TOP_PANEL_HEIGHT 10
#define BOTTOM_PANEL_HEIGHT 70
#define VISIBLE_TILES_X (MAP_WIDTH >> TILE_SHIFT)
#define VISIBLE_TILES_Y (MAP_HEIGHT >> TILE_SHIFT)

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

void loadMap(const char* name, uint16_t tilebufCoplistStart, uint16_t tilebufCoplistBreak, uint16_t mapColorsCoplistStart) {
    char* mapname = MAPDIR LONGEST_MAPNAME;
    char* palname = IMGDIR "for.plt";
    char* imgname = IMGDIR "for.bm";

    snprintf(mapname + strlen(MAPDIR), strlen(LONGEST_MAPNAME) + 1, "%s.map", name);
    tFile *map = fileOpen(mapname, "r");
    if (!map) {
        logWrite("ERROR: Cannot open file %s!\n", mapname);
    }

    // first three bytes are simply name of the palette/terrain
    fileRead(map, palname + strlen(IMGDIR), 3);
    strncpy(imgname + strlen(IMGDIR), palname + strlen(IMGDIR), 3);

    logWrite("Loading map: %s %s\n", palname, imgname);
    // create map area
    paletteLoad(palname, s_pMapPalette, COLORS);
    tCopCmd *pCmds = &s_pView->pCopList->pBackBfr->pList[mapColorsCoplistStart];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[mapColorsCoplistStart];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }

    s_pMapBitmap = bitmapCreateFromFile(imgname, 0);
    s_pMapBuffer = tileBufferCreate(0,
                                    TAG_TILEBUFFER_VPORT, s_pVpMain,
                                    TAG_TILEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                    TAG_TILEBUFFER_BOUND_TILE_X, MAP_SIZE,
                                    TAG_TILEBUFFER_BOUND_TILE_Y, MAP_SIZE,
                                    TAG_TILEBUFFER_TILE_SHIFT, TILE_SHIFT,
                                    TAG_TILEBUFFER_TILESET, s_pMapBitmap,
                                    TAG_TILEBUFFER_IS_DBLBUF, 1,
                                    TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 6,
                                    TAG_TILEBUFFER_COPLIST_OFFSET_START, tilebufCoplistStart,
                                    TAG_TILEBUFFER_COPLIST_OFFSET_BREAK, tilebufCoplistBreak,
                                    TAG_END);
    s_pMainCamera = s_pMapBuffer->pCamera;
    cameraSetCoord(s_pMainCamera, 0, 0);

    for (int x = 0; x < MAP_SIZE; x++) {
        fileRead(map, s_pMapBuffer->pTileData[x], MAP_SIZE);
    }
    fileClose(map);

    tileBufferRedrawAll(s_pMapBuffer);
}

void initBobs(void) {
    bobNewManagerCreate(s_pMapBuffer->pScroll->pFront, s_pMapBuffer->pScroll->pBack, s_pMapBuffer->pScroll->uwBmAvailHeight);

    bobNewInit(&s_TileCursor, TILE_SIZE, TILE_SIZE, 1, s_pMapBitmap, 0, 0, 0);
    bobNewSetBitMapOffset(&s_TileCursor, 0x10 << TILE_SHIFT);

    s_pUnitManager = unitManagerCreate();
    unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 7, .ubY = 7});
    unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 8, .ubY = 7});
    unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 7, .ubY = 8});
    unitSetTilePosition(unitNew(s_pUnitManager, spearman), s_pMapBuffer->pTileData, (tUbCoordYX){.ubX = 8, .ubY = 8});

    bobNewReallocateBgBuffers();
}

void loadUi(uint16_t topPanelColorsPos, uint16_t panelColorsPos, uint16_t simplePosTop, uint16_t simplePosBottom) {
    // create panel area
    paletteLoad("resources/ui/panel.plt", s_pPanelPalette, COLORS);

    tCopCmd *pCmds = &s_pView->pCopList->pBackBfr->pList[topPanelColorsPos];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[topPanelColorsPos];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }

    pCmds = &s_pView->pCopList->pBackBfr->pList[panelColorsPos];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[panelColorsPos];
    for (uint8_t i = 0; i < COLORS; i++) {
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
    uint16_t spritePos = 0;
    uint16_t selectionPos = spritePos + mouseSpriteGetRawCopplistInstructionCountLength();
    uint16_t simplePosTop = selectionPos + selectionSpritesGetRawCopplistInstructionCountLength();
    uint16_t topPanelColorsPos = simplePosTop + simpleBufferGetRawCopperlistInstructionCount(BPP);
    uint16_t tilebufCoplistStart = topPanelColorsPos + COLORS;
    uint16_t mapColorsCoplistStart = tilebufCoplistStart + tileBufferGetRawCopperlistInstructionCountStart(BPP);
    uint16_t tilebufCoplistBreak = mapColorsCoplistStart + COLORS;
    uint16_t simplePosBottom = tilebufCoplistBreak + tileBufferGetRawCopperlistInstructionCountBreak(BPP);
    uint16_t panelColorsPos = simplePosBottom + simpleBufferGetRawCopperlistInstructionCount(BPP);
    uint16_t copListLength = panelColorsPos + COLORS;

    // Create the game view
    s_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, copListLength,
                         TAG_DONE);

    // setup mouse
    mouseSpriteSetup(s_pView, spritePos);

    // setup selection rectangles
    selectionSpritesSetup(s_pView, selectionPos);

    createViewports();

    // load map file
    loadMap("game2", tilebufCoplistStart, tilebufCoplistBreak, mapColorsCoplistStart);

    initBobs();

    loadUi(topPanelColorsPos, panelColorsPos, simplePosTop, simplePosBottom);

    viewLoad(s_pView);

    systemSetDmaMask(DMAF_SPRITE, 1);

    systemUnuse();
}

static inline tUbCoordYX screenPosToTile(tUwCoordYX pos) {
    return (tUbCoordYX){.ubX = pos.uwX >> TILE_SHIFT, .ubY = pos.uwY >> TILE_SHIFT};
}

void handleInput(tUwCoordYX mousePos) {
    static tUwCoordYX lmbDown = {.ulYX = 0};

    if (keyCheck(KEY_ESCAPE)) {
        gameExit();
    } else if (keyCheck(KEY_C)) {
        copDumpBfr(s_pView->pCopList->pBackBfr);
    } else if (keyCheck(KEY_E)) {
        s_Mode = edit;
    } else if (keyCheck(KEY_G)) {
        s_Mode = game;
    } else if (keyCheck(KEY_UP)) {
        cameraMoveBy(s_pMainCamera, 0, -4);
    } else if (keyCheck(KEY_DOWN)) {
        cameraMoveBy(s_pMainCamera, 0, 4);
    } else if (keyCheck(KEY_LEFT)) {
        cameraMoveBy(s_pMainCamera, -4, 0);
    } else if (keyCheck(KEY_RIGHT)) {
        cameraMoveBy(s_pMainCamera, 4, 0);
    } else if (keyCheck(KEY_SPACE)) {
        if (s_pMainCamera->uPos.ulYX == 0) {
            cameraSetCoord(s_pMainCamera, 512, 512);
        } else {
            cameraSetCoord(s_pMainCamera, 0, 0);
        }
    }
    if (!mousePos.uwY) {
        cameraMoveBy(s_pMainCamera, 0, -4);
    } else if (mousePos.uwY >= MAP_HEIGHT + BOTTOM_PANEL_HEIGHT - 1) {
        cameraMoveBy(s_pMainCamera, 0, 4);
    }
    if (!mousePos.uwX) {
        cameraMoveBy(s_pMainCamera, -4, 0);
    } else if (mousePos.uwX > MAP_WIDTH - 1) {
        cameraMoveBy(s_pMainCamera, 4, 0);
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
        uint8_t tmp = tile1.ubX - tile2.ubX;
        uint8_t operand = tmp & (tmp >> (sizeof(int) * CHAR_BIT - 1));
        uint8_t x1 = tile2.ubX + operand; // min(x, y)
        uint8_t x2 = tile1.ubX - operand; // max(x, y)
        tmp = tile1.ubY - tile2.ubY;
        operand = tmp & (tmp >> (sizeof(int) * CHAR_BIT - 1));
        uint8_t y1 = tile2.ubY + operand; // min(x, y)
        uint8_t y2 = tile1.ubY - operand; // max(x, y)
        tmp = 0;
        while (y1 <= y2) {
            uint8_t xcur = x1;
            while (xcur <= x2) {
                tUbCoordYX tile = (tUbCoordYX){.ubX = xcur, .ubY = y1};
                Unit *unit = unitManagerUnitAt(s_pUnitManager, tile);
                if (unit) {
                    if (keyCheck(KEY_LSHIFT) || keyCheck(KEY_RSHIFT) || tmp) {
                        for(uint8_t idx = 0; idx < NUM_SELECTION; ++idx) {
                            Unit *sel = s_pSelectedUnit[idx];
                            if (!sel) {
                                s_pSelectedUnit[idx] = unit;
                                break;
                            }
                            if (sel == unit) {
                                break;
                            }
                        }
                    } else {
                        s_pSelectedUnit[0] = unit;
                        for(uint8_t idx = 1; idx < NUM_SELECTION; ++idx) {
                            s_pSelectedUnit[idx] = NULL;
                        }
                    }
                    ++tmp;
                }
                ++xcur;
            }
            ++y1;
        }
        if (!tmp) {
            for(uint8_t idx = 0; idx < NUM_SELECTION; ++idx) {
                s_pSelectedUnit[idx] = NULL;
            }
        }
        lmbDown.ulYX = 0;
        selectionRectangleUpdate(-1, -1, -1, -1);
    } else if (s_pSelectedUnit[0] && mouseCheck(MOUSE_PORT_1, MOUSE_RMB)) {
        tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + s_pMainCamera->uPos.ulYX});
        for(uint8_t idx = 0; idx < NUM_SELECTION; ++idx) {
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
    for(uint8_t idx = 0; idx < NUM_SELECTION && (unit = s_pSelectedUnit[idx]); ++idx) {
        tIcon *icon = &s_panelUnitIcons[idx];
        iconSetSource(icon, s_pIcons, UnitTypes[unit->type].iconIdx);
        iconDraw(icon);
    }
}

void drawSelectionRectangles(void) {
    for(uint8_t idx = 0; idx < NUM_SELECTION; ++idx) {
        Unit *unit = s_pSelectedUnit[idx];
        if (unit) {
            int16_t bobPosOnScreenX = s_pSelectedUnit[idx]->bob.sPos.uwX - s_pMainCamera->uPos.uwX;
            if (bobPosOnScreenX >= -8) {
                int16_t bobPosOnScreenY = s_pSelectedUnit[idx]->bob.sPos.uwY - s_pMainCamera->uPos.uwY + TOP_PANEL_HEIGHT;
                if (bobPosOnScreenX >= -8) {
                    selectionSpritesUpdate(idx, bobPosOnScreenX + 8, bobPosOnScreenY + 8);
                    continue;
                }
            }
        }
        selectionSpritesUpdate(idx, -1, -1);
    }
}

void gameGsLoop(void) {
    UWORD mouseX = mouseGetX(MOUSE_PORT_1);
    UWORD mouseY = mouseGetY(MOUSE_PORT_1);
    tUwCoordYX mousePos = {.uwX = mouseX & 0xfff0, .uwY = mouseY & 0xfff0};

    if (s_Mode == edit) {
        if (!(GameCycle % 10)) {
            if (keyCheck(KEY_UP)) {
                SelectedTile++;
                bobNewSetBitMapOffset(&s_TileCursor, SelectedTile << TILE_SHIFT);
            } else if (keyCheck(KEY_DOWN)) {
                SelectedTile--;
                bobNewSetBitMapOffset(&s_TileCursor, SelectedTile << TILE_SHIFT);
            } else if (keyCheck(KEY_RETURN)) {
                const char* mapname = MAPDIR "game.map";
                tFile *map = fileOpen(mapname, "w");
                if (!map) {
                    logWrite("ERROR: Cannot open file %s!\n", mapname);
                } else {
                    fileWrite(map, "for", 3);
                    for (int x = 0; x < MAP_SIZE; x++) {
                        fileWrite(map, s_pMapBuffer->pTileData[x], MAP_SIZE);
                    }
                }
            }
        }
        if (mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            tUbCoordYX tile = screenPosToTile((tUwCoordYX){.ulYX = mousePos.ulYX + s_pMainCamera->uPos.ulYX});
            tileBufferSetTile(s_pMapBuffer, tile.ubX, tile.ubY, SelectedTile);
            tileBufferQueueProcess(s_pMapBuffer);
            bobNewDiscardUndraw();
        }
    }

    bobNewBegin(s_pMapBuffer->pScroll->pBack);

    // process all units
    tUbCoordYX tileTopLeft = screenPosToTile(s_pMainCamera->uPos);
    unitManagerProcessUnits(
        s_pUnitManager,
        s_pMapBuffer->pTileData,
        tileTopLeft,
        (tUbCoordYX){.ubX = tileTopLeft.ubX + VISIBLE_TILES_X, .ubY = tileTopLeft.ubY + VISIBLE_TILES_Y}
    );

    if (s_Mode == edit) {
        s_TileCursor.sPos.ulYX = mousePos.ulYX;
        bobNewPush(&s_TileCursor);
    }

    bobNewEnd();

    tileBufferQueueProcess(s_pMapBuffer);
    viewProcessManagers(s_pView);
    copSwapBuffers();
    // never process the panel buffer, it doesn't do anything

    handleInput(mousePos);

    uint8_t renderCycle = GameCycle & 0b1111;
    switch (renderCycle) {
        case 0:
            drawPanel();
            break;
        default:
            break;
    }

    vPortWaitForEnd(s_pVpPanel);

    mouseSpriteUpdate(mouseX, mouseY);
    drawSelectionRectangles();

    GameCycle++;
}

void gameGsDestroy(void) {
    systemUse();

    // This will also destroy all associated viewports and viewport managers
    viewDestroy(s_pView);

    unitManagerDestroy(s_pUnitManager);

    bobNewManagerDestroy();
    bitmapDestroy(s_pMapBitmap);
}
