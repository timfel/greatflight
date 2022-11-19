#include "game.h"
#include "bob_new.h"
#include "map.h"
#include "units.h"
#include "actions.h"
#include "mouse_sprite.h"
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
#include <stdint.h>

#include <graphics/sprite.h>

#define BPP 5
#define COLORS (1 << BPP)

static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpPanel; // Viewport for panel
static tSimpleBufferManager *s_pPanelBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tTileBufferManager *s_pMapBuffer;
static tCameraManager *s_pMainCamera;
static tBitMap *s_pMapBitmap;

static tBobNew s_TileCursor;

static Unit *s_pSpearman;

// palette switching
static uint16_t s_pMapPalette[COLORS];
// static uint16_t s_pPanelPalette[COLORS];

#define IMGDIR "resources/imgs/"
#define MAPDIR "resources/maps/"
#define LONGEST_MAPNAME "human12.map"

#define MAP_HEIGHT 200
#define PANEL_HEIGHT 48

static UWORD s_uwTilebufCoplistStart;
static UWORD s_uwMapColorsCoplistStart;
static UWORD s_uwTilebufCoplistBreak;

void loadMap(const char* name) {
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
    tCopCmd *pCmds = &s_pView->pCopList->pBackBfr->pList[s_uwMapColorsCoplistStart];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }
    pCmds = &s_pView->pCopList->pFrontBfr->pList[s_uwMapColorsCoplistStart];
    for (uint8_t i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pMapPalette[i]);
    }

    s_pVpMain = vPortCreate(0,
                            TAG_VPORT_VIEW, s_pView,
                            TAG_VPORT_BPP, BPP,
                            TAG_VPORT_HEIGHT, MAP_HEIGHT,
                            TAG_END);
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
                                    TAG_TILEBUFFER_COPLIST_OFFSET_START, s_uwTilebufCoplistStart,
                                    TAG_TILEBUFFER_COPLIST_OFFSET_BREAK, s_uwTilebufCoplistBreak,
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

    unitManagerCreate();
    s_pSpearman = unitNew(spearman);

    bobNewReallocateBgBuffers();
}

void gameGsCreate(void) {
    viewLoad(0);

    uint16_t spritePos = 0;
    s_uwTilebufCoplistStart = spritePos + mouseSpriteGetRawCopplistInstructionCountLength();
    s_uwMapColorsCoplistStart = s_uwTilebufCoplistStart + tileBufferGetRawCopperlistInstructionCountStart(BPP);
    s_uwTilebufCoplistBreak = s_uwMapColorsCoplistStart + COLORS;
    uint16_t simplePos = s_uwTilebufCoplistBreak + tileBufferGetRawCopperlistInstructionCountBreak(BPP);
    uint16_t panelColorsPos = simplePos + simpleBufferGetRawCopperlistInstructionCount(BPP);
    uint16_t copListLength = panelColorsPos;

    s_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, copListLength,
                         TAG_DONE);

    mouseSpriteSetup(s_pView, spritePos);

    loadMap("game2");

    initBobs();

    // create panel area
    // paletteLoad("resources/human_panel.plt", s_pPanelPalette, COLORS);
    // pCmds = &pCopBfrBack->pList[panelColorsPos];
    // for (uint8_t i = 0; i < COLORS; i++) {
    //     copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    // }
    // pCmds = &pCopBfrFront->pList[panelColorsPos];
    // for (uint8_t i = 0; i < COLORS; i++) {
    //     copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], s_pPanelPalette[i]);
    // }

    s_pVpPanel = vPortCreate(0,
                             TAG_VPORT_VIEW, s_pView,
                             TAG_VPORT_BPP, BPP,
                             // TAG_VPORT_OFFSET_TOP, 1,
                             TAG_VPORT_HEIGHT, PANEL_HEIGHT,
                             TAG_END);
    logWrite("create panel buffer\n");
    s_pPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, s_pVpPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, simplePos,
                                        TAG_END);
    // bitmapLoadFromFile(s_pPanelBuffer->pFront, "resources/human_panel/graphics/ui/human/panel_2.bm", 48, 0);

    viewLoad(s_pView);

    systemSetDmaMask(DMAF_SPRITE, 1);

    systemUnuse();
}

static inline tUbCoordYX screenPosToTile(UWORD x, UWORD y) {
    tUbCoordYX pos;
    pos.ubX = x >> TILE_SHIFT;
    pos.ubY = y >> TILE_SHIFT;
    return pos;
}

enum mode {
    game,
    edit
};
static uint16_t GameCycle = 0;
static UBYTE s_Mode = game;

// editor statics
static UBYTE SelectedTile = 0x10;

// game statics
static Unit *s_pSelectedUnit = NULL;
static UBYTE s_ubCurrentlyHandledUnit = 0;

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
            tUbCoordYX tile = screenPosToTile(mousePos.uwX, mousePos.uwY);
            tileBufferSetTile(s_pMapBuffer, tile.ubX, tile.ubY, SelectedTile);
            tileBufferQueueProcess(s_pMapBuffer);
            bobNewDiscardUndraw();
        }
    } else {
        // mode == game
        if (mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
            tUbCoordYX tile = screenPosToTile(mousePos.uwX, mousePos.uwY);
            for (UBYTE i = 0; i < MAX_UNITS; i++) {
                Unit *unit = s_pUnitList[i];
                if (unit != NULL) {
                    tUbCoordYX loc = unitGetTilePosition(unit);
                    if (loc.uwYX == tile.uwYX) {
                        s_pSelectedUnit = unit;
                        break;
                    }
                }
            }
        } else if (s_pSelectedUnit && mouseCheck(MOUSE_PORT_1, MOUSE_RMB)) {
            actionMoveTo(s_pSelectedUnit, screenPosToTile(mousePos.uwX, mousePos.uwY));
        }
    }

    // This will loop every frame
    if (keyCheck(KEY_ESCAPE)) {
        gameExit();
    } else if (keyCheck(KEY_C)) {
        copDumpBfr(s_pView->pCopList->pBackBfr);
    } else if (keyCheck(KEY_E)) {
        s_Mode = edit;
    } else if (keyCheck(KEY_G)) {
        s_Mode = game;
    } else if (keyCheck(KEY_W)) {
        cameraMoveBy(s_pMainCamera, 0, -4);
    } else if (keyCheck(KEY_S)) {
        cameraMoveBy(s_pMainCamera, 0, 4);
    } else if (keyCheck(KEY_A)) {
        cameraMoveBy(s_pMainCamera, -4, 0);
    } else if (keyCheck(KEY_D)) {
        cameraMoveBy(s_pMainCamera, 4, 0);
    }

    UBYTE ubNextStop = s_ubCurrentlyHandledUnit + 20;
    for (; s_ubCurrentlyHandledUnit < ubNextStop; s_ubCurrentlyHandledUnit++) {
        Unit *unit = s_pUnitList[s_ubCurrentlyHandledUnit];
        if (unit) {
            actionDo(unit, s_pMapBuffer->pTileData);
        }
    }
    if (ubNextStop >= MAX_UNITS) {
        s_ubCurrentlyHandledUnit = 0;
    }

    bobNewBegin(s_pMapBuffer->pScroll->pBack);
    
    for (UBYTE i = 0; i < MAX_UNITS; i++) {
        Unit *unit = s_pUnitList[i];
        if (unit != NULL) {
            tUbCoordYX loc = unitGetTilePosition(unit);
            if (tileBufferIsTileOnBuffer(s_pMapBuffer, loc.ubX, loc.ubY)) {
                unitDraw(unit);
            }
        }
    }

    if (s_Mode == edit) {
        s_TileCursor.sPos.ulYX = mousePos.ulYX;
        bobNewPush(&s_TileCursor);
    }

    bobNewEnd();

    viewProcessManagers(s_pView);
    copProcessBlocks();
    vPortWaitForEnd(s_pVpPanel);

    mouseSpriteUpdate(mouseX, mouseY);

    GameCycle++;
}

void gameGsDestroy(void) {
    systemUse();

    // This will also destroy all associated viewports and viewport managers
    viewDestroy(s_pView);

    unitDelete(s_pSpearman);
    unitManagerDestroy();

    bobNewManagerDestroy();
    bitmapDestroy(s_pMapBitmap);
}
