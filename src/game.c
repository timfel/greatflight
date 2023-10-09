#include <limits.h>

#include "game.h"
#include "include/map.h"
#include "include/player.h"
#include "include/units.h"
#include "include/resources.h"
#include "actions.h"
#include "mouse_sprite.h"
#include "selection_sprites.h"

#include <ace/utils/font.h>
#include <graphics/sprite.h>

#define VISIBLE_TILES_X (MAP_WIDTH >> TILE_SHIFT)
#define VISIBLE_TILES_Y (MAP_HEIGHT >> TILE_SHIFT)
#define CAMERA_MOVE_DELTA PATHMAP_TILE_SIZE

struct Screen g_Screen;
static tBob s_TileCursor;
static tUnitManager *s_pUnitManager;

static tFont *font54;

enum mode {
    game,
    edit
};
static UWORD GameCycle = 0;
static UBYTE s_Mode = game;

// editor statics
static UBYTE SelectedTile = 0x10;

struct copOffsets_t {
    UWORD spritePos;
    UWORD selectionPos;
    UWORD simplePosTop;
    UWORD topPanelColorsPos;
    UWORD mapbufCoplistStart;
    UWORD mapColorsCoplistStart;
    UWORD simplePosBottom;
    UWORD panelColorsPos;
    UWORD copListLength;
};
static struct copOffsets_t s_copOffsets;

// game statics
static Unit *s_pSelectedUnit[NUM_SELECTION];

void loadFonts() {
    font54 = fontCreate("resources/ui/uni54.fnt");
}

void createViewports() {
    g_Screen.m_panels.m_pTopPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, g_Screen.m_pView,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, TOP_PANEL_HEIGHT,
                             TAG_END);
    g_Screen.m_map.m_pVPort = vPortCreate(0,
                            TAG_VPORT_VIEW, g_Screen.m_pView,
                            TAG_VPORT_OFFSET_TOP, UWORD_MAX - 1,
                            TAG_VPORT_BPP, BPP,
                            TAG_VPORT_WIDTH, MAP_WIDTH,
                            TAG_VPORT_HEIGHT, MAP_HEIGHT,
                            TAG_END);
    g_Screen.m_panels.m_pMainPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, g_Screen.m_pView,
                             TAG_VPORT_OFFSET_TOP, UWORD_MAX - 1,
                             TAG_VPORT_BPP, BPP,
                             TAG_VPORT_WIDTH, MAP_WIDTH,
                             TAG_VPORT_HEIGHT, BOTTOM_PANEL_HEIGHT,
                             TAG_END);
}

ULONG tileIndexToTileBitmapOffset(UBYTE index) {
    return (ULONG)(g_Screen.m_map.m_pTilemap->Planes[0] + (g_Screen.m_map.m_pTilemap->BytesPerRow * (index << TILE_SHIFT)));
}

void loadTileBitmap() {
    g_Screen.m_map.m_pTilemap = bitmapCreateFromFile(g_Map.m_pTileset, 0);
}

void loadMap(const char* name, UWORD mapbufCoplistStart, UWORD mapColorsCoplistStart) {
    char* mapname = MAPDIR LONGEST_MAPNAME;

    snprintf(mapname + strlen(MAPDIR), strlen(LONGEST_MAPNAME) + 1, "%s.map", name);
    tFile *map = fileOpen(mapname, "r");
    if (!map) {
        logWrite("ERROR: Cannot open file %s!\n", mapname);
    }

    mapLoad(map, &loadTileBitmap);
    playersLoad(map);
    unitsLoad(s_pUnitManager, map);
    fileClose(map);

    // now setup map viewport
    char* palname = PALDIR "for.plt";
    strncpy(palname + strlen(PALDIR), g_Map.m_pTileset + strlen(TILESETDIR), 3);
    paletteLoad(palname, g_Screen.m_map.m_pPalette, COLORS);
    tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[mapColorsCoplistStart];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }
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

    // g_Screen.m_pIcons = bitmapCreateFromFile("resources/ui/icons.bm", 0);

    // iconInit(&g_Screen.m_pUnitIcons[0], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 18});
    // iconInit(&g_Screen.m_pUnitIcons[1], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 18});
    // iconInit(&g_Screen.m_pUnitIcons[2], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 88, .uwY = 44});
    // iconInit(&g_Screen.m_pUnitIcons[3], 32, 26, g_Screen.m_pIcons, 0, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 150, .uwY = 44});
}

void gameGsCreate(void) {
    viewLoad(0);

    // Calculate copperlist size
    s_copOffsets.spritePos = 0;
    s_copOffsets.selectionPos = s_copOffsets.spritePos + mouseSpriteGetRawCopplistInstructionCountLength();
    s_copOffsets.simplePosTop = s_copOffsets.selectionPos + selectionSpritesGetRawCopplistInstructionCountLength();
    s_copOffsets.topPanelColorsPos = s_copOffsets.simplePosTop + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.mapbufCoplistStart = s_copOffsets.topPanelColorsPos + COLORS;
    s_copOffsets.mapColorsCoplistStart = s_copOffsets.mapbufCoplistStart + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.simplePosBottom = s_copOffsets.mapColorsCoplistStart + COLORS;
    s_copOffsets.panelColorsPos = s_copOffsets.simplePosBottom + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.copListLength = s_copOffsets.panelColorsPos + COLORS;

    // Create the game view
    g_Screen.m_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, s_copOffsets.copListLength,
                         TAG_VIEW_WINDOW_START_Y, SCREEN_PAL_YOFFSET,
                         TAG_VIEW_WINDOW_HEIGHT, TOP_PANEL_HEIGHT + MAP_HEIGHT + BOTTOM_PANEL_HEIGHT,
                         TAG_DONE);

    // setup mouse
    mouseSpriteSetup(g_Screen.m_pView, s_copOffsets.spritePos);

    // setup selection rectangles
    selectionSpritesSetup(g_Screen.m_pView, s_copOffsets.selectionPos);

    createViewports();

    s_pUnitManager = unitManagerCreate();

    // load map file
    loadMap(g_Map.m_pName, s_copOffsets.mapbufCoplistStart, s_copOffsets.mapColorsCoplistStart);

    initBobs();

    loadUi(s_copOffsets.topPanelColorsPos, s_copOffsets.panelColorsPos, s_copOffsets.simplePosTop, s_copOffsets.simplePosBottom);

    loadFonts();

    viewLoad(g_Screen.m_pView);

    systemSetDmaMask(DMAF_SPRITE, 1);

    systemUnuse();
}

/**
 * Screen coordinates to pathmap coordinates
 */
static inline tUbCoordYX screenPosToTile(tUwCoordYX pos) {
    return (tUbCoordYX){.ubX = pos.uwX / PATHMAP_TILE_SIZE, .ubY = (pos.uwY - TOP_PANEL_HEIGHT) / PATHMAP_TILE_SIZE};
}

void drawInfoPanel(void) {
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
            // tIcon *icon = &g_Screen.m_pUnitIcons[idx];
            // iconSetSource(icon, g_Screen.m_pIcons, UnitTypes[unit->type].iconIdx);
            // iconDraw(icon);
        }
    }
}

void drawSelectionRectangles(void) {
    for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
        Unit *unit = s_pSelectedUnit[idx];
        if (unit) {
            WORD bobPosOnScreenX = unit->bob.sPos.uwX - g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX;
            BYTE offset = UnitTypes[unit->type].anim.large ? 8 : 0;
            if (bobPosOnScreenX >= -offset && bobPosOnScreenX <= MAP_WIDTH + offset) {
                WORD bobPosOnScreenY = unit->bob.sPos.uwY - g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY;
                if (bobPosOnScreenY >= -offset && bobPosOnScreenY <= MAP_HEIGHT + offset) {
                    selectionSpritesUpdate(idx, bobPosOnScreenX, bobPosOnScreenY + TOP_PANEL_HEIGHT + 1);
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
	UBYTE ubEndX = ubStartX + (MAP_BUFFER_WIDTH >> TILE_SHIFT);

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
        ++pTileBitmapOffset;
        blitWait();
        g_pCustom->bltapt = (APTR)*pTileBitmapOffset;
        g_pCustom->bltsize = uwBltsize;
        pDstPlane += (TILE_SIZE >> 3);
    }
    systemSetDmaBit(DMAB_BLITHOG, 0);
}

void logicLoop(void) {
    GameCycle++;
}

void minimapUpdate(void) {
}

void handleInput() {
    UWORD mouseX = mouseGetX(MOUSE_PORT_1);
    UWORD mouseY = mouseGetY(MOUSE_PORT_1);
    static tUwCoordYX lmbDown = {.ulYX = 0};
    
    tUwCoordYX mousePos = {.uwX = mouseX, .uwY = mouseY};

    // XXX: menu buttons
    static UWORD topHighlight = 0;
    // XXX: matches the generated gpl file index for the color i want to change, this is pretty random
    static UBYTE blue1 = 10; static UWORD red1 = 0xd45;
    static UBYTE blue2 = 6; static UWORD red2 = 0xa44;
    static UBYTE blue3 = 5; static UWORD red3 = 0x724;
    if (mouseY < TOP_PANEL_HEIGHT && mouseX <= 60) {
        topHighlight = 1;
        tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.topPanelColorsPos];
        copSetMoveVal(&pCmds[blue1].sMove, red1);
        copSetMoveVal(&pCmds[blue2].sMove, red2);
        copSetMoveVal(&pCmds[blue3].sMove, red3);
        pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.topPanelColorsPos];
        copSetMoveVal(&pCmds[blue1].sMove, red1);
        copSetMoveVal(&pCmds[blue2].sMove, red2);
        copSetMoveVal(&pCmds[blue3].sMove, red3);
    } else if (topHighlight) {
        topHighlight = 0;
        tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.topPanelColorsPos];
        copSetMoveVal(&pCmds[blue1].sMove, g_Screen.m_panels.m_pPalette[blue1]);
        copSetMoveVal(&pCmds[blue2].sMove, g_Screen.m_panels.m_pPalette[blue2]);
        copSetMoveVal(&pCmds[blue3].sMove, g_Screen.m_panels.m_pPalette[blue3]);
        pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.topPanelColorsPos];
        copSetMoveVal(&pCmds[blue1].sMove, g_Screen.m_panels.m_pPalette[blue1]);
        copSetMoveVal(&pCmds[blue2].sMove, g_Screen.m_panels.m_pPalette[blue2]);
        copSetMoveVal(&pCmds[blue3].sMove, g_Screen.m_panels.m_pPalette[blue3]);
    }
    // END XXX

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
            g_Map.m_ulTilemapXY[tile.ubX][tile.ubY] = tileIndexToTileBitmapOffset(SelectedTile);
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
                selectionRectangleUpdate(lmbDown.uwX, mousePos.uwX, lmbDown.uwY, mousePos.uwY);
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

void colorCycle(void) {
    if ((GameCycle & 31) == 31) {
        if (GameCycle & 32) {
            // XXX: hardcoded gpl indices
            tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
            pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
        } else if (GameCycle & 64) {
            // XXX: hardcoded gpl indices
            tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
            pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
        } else {
            tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[12]);
            pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
            copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[12]);
        }
    }
}

void fowUpdate(void) {
}

void drawResources(void) {
    if ((GameCycle & 63) == 63) {
        // TODO: only store and redraw the dirty part
        // g_Screen.m_ubTopPanelDirty = 0;
        // // the panel is never actually swapped, the backbuffer is just the plain panel for redraw
        // blitWait(); // Don't modify registers when other blit is in progress
        // g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        // g_pCustom->bltcon1 = 0;
        // g_pCustom->bltafwm = 0xffff;
        // g_pCustom->bltalwm = 0xffff;
        // g_pCustom->bltamod = 0;
        // g_pCustom->bltdmod = 0;
        // g_pCustom->bltapt = g_Screen.m_panels.s_pTopPanelBackground->Planes[0];
        // g_pCustom->bltdpt = g_Screen.m_panels.m_pTopPanelBuffer->pFront->Planes[0];
        // g_pCustom->bltsize = ((TOP_PANEL_HEIGHT * BPP) << 6) | (MAP_WIDTH >> 4);

        static tTextBitMap *goldBitmap = NULL;
        static tTextBitMap *lumberBitmap = NULL;
        static UWORD gold = -1;
        static UWORD lumber = -1;
        static char str[7] = {'\0'};
        if (GameCycle & 64) {
            if (gold != g_pPlayers[0].gold) {
                gold = g_pPlayers[0].gold;
                snprintf(str, 6, "%d", gold);
                goldBitmap = fontCreateTextBitMapFromStr(font54, str);
            }
            fontDrawTextBitMap(g_Screen.m_panels.m_pTopPanelBuffer->pBack, goldBitmap, 128, 0, 1, 0);
        } else {
            if (lumber != g_pPlayers[0].lumber) {
                lumber = g_pPlayers[0].lumber;
                snprintf(str, 6, "%d", lumber);
                lumberBitmap = fontCreateTextBitMapFromStr(font54, str);
            }
            fontDrawTextBitMap(g_Screen.m_panels.m_pTopPanelBuffer->pBack, lumberBitmap, 232, 0, 1, 0);
        }
    }
}

void drawMissiles(void) {
}

void drawUnits(void) {
    // process all units
    tUbCoordYX tileTopLeft = screenPosToTile(g_Screen.m_map.m_pCamera->uPos);
    unitManagerProcessUnits(
        s_pUnitManager,
        g_Map.m_ubPathmapXY,
        tileTopLeft,
        (tUbCoordYX){.ubX = (tileTopLeft.ubX + VISIBLE_TILES_X * 2), .ubY = (tileTopLeft.ubY + VISIBLE_TILES_Y * 2)}
    );
}

void drawFog(void) {
}

void drawMap(void) {
    drawAllTiles();
    bobBegin(g_Screen.m_map.m_pBuffer->pBack);
    drawUnits();
    drawMissiles();
    if (s_Mode == edit) {
        bobPush(&s_TileCursor);
    }
    bobEnd();
    drawFog();
}

void drawMessages(void) {
}

void drawActionButtons(void) {
}

void drawMenuButton(void) {
}

void drawMinimap(void) {
}

void drawStatusLine(void) {
}

void updateDisplay(void) {
    drawMap();
    drawMessages();
    drawMenuButton();
    drawActionButtons();
    drawMinimap();
    drawInfoPanel();
    drawResources();
    drawStatusLine();

    // finish frame
    viewProcessManagers(g_Screen.m_pView);
    copSwapBuffers();
    vPortWaitUntilEnd(g_Screen.m_panels.m_pMainPanel);

    // update sprites during vblank
    mouseSpriteUpdate(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
    drawSelectionRectangles();
}

void displayLoop(void) {
    minimapUpdate();
    handleInput();
    colorCycle();
    fowUpdate();
    updateDisplay();
}

void gameGsLoop(void) {
    displayLoop();
    logicLoop();
}

void gameGsDestroy(void) {
    systemUse();

    // This will also destroy all associated viewports and viewport managers
    viewDestroy(g_Screen.m_pView);

    unitManagerDestroy(s_pUnitManager);

    bobManagerDestroy();
    bitmapDestroy(g_Screen.m_map.m_pTilemap);
}
