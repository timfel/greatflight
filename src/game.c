#include <limits.h>

#include "game.h"
#include "include/map.h"
#include "include/player.h"
#include "include/units.h"
#include "include/buildings.h"
#include "include/resources.h"
#include "include/sprites.h"

#include <graphics/sprite.h>

#define VISIBLE_TILES_X (MAP_WIDTH >> TILE_SHIFT)
#define VISIBLE_TILES_Y (MAP_HEIGHT >> TILE_SHIFT)
#define CAMERA_MOVE_DELTA 4
#define RESOURCE_DIGITS 6

struct Screen g_Screen;

static UWORD s_mouseX;
static UWORD s_mouseY;
static tUwRect s_selectionRubberBand;

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

UBYTE tileBitmapOffsetToTileIndex(ULONG offset) {
    return ((offset - (ULONG)g_Screen.m_map.m_pTilemap->Planes[0]) / g_Screen.m_map.m_pTilemap->BytesPerRow) >> TILE_SHIFT;
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
    unitsLoad(map);
    fileClose(map);

    // now setup map viewport
    char* palname = PALDIR "for.plt";
    strncpy(palname + strlen(PALDIR), g_Map.m_pTileset + strlen(TILESETDIR), 3);
    paletteLoad(palname, g_Screen.m_map.m_pPalette, COLORS + 1);
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
                                    TAG_SIMPLEBUFFER_USE_X_SCROLLING, 1,
                                    TAG_END);
    g_Screen.m_map.m_pCamera = cameraCreate(g_Screen.m_map.m_pVPort, 0, 0, MAP_SIZE * TILE_SIZE, MAP_SIZE * TILE_SIZE, 0);
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
    iconInit(&g_Screen.m_pActionIcons[0], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_MOVE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 208, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[1], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_STOP, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 241, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[2], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_ATTACK, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 274, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[3], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 208, .uwY = 46});
    iconInit(&g_Screen.m_pActionIcons[4], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 241, .uwY = 46});
    iconInit(&g_Screen.m_pActionIcons[5], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 274, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[0], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 80, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[1], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 113, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[2], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 146, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[3], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 80, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[4], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 113, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[5], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 146, .uwY = 46});
}

void screenInit(void) {
    memset(&g_Screen, 0, sizeof(struct Screen));

    g_Screen.m_ubTopPanelDirty = 1;
    g_Screen.m_ubBottomPanelDirty = 1;

    g_Screen.m_fonts.m_pNormalFont = fontCreate("resources/ui/uni54.fnt");
    const char text[RESOURCE_DIGITS + 1] = "000000";
    g_Screen.m_panels.m_pGoldTextBitmap = fontCreateTextBitMapFromStr(g_Screen.m_fonts.m_pNormalFont, text);
    g_Screen.m_panels.m_pLumberTextBitmap = fontCreateTextBitMapFromStr(g_Screen.m_fonts.m_pNormalFont, text);
    g_Screen.m_panels.m_pUnitNameBitmap = fontCreateTextBitMapFromStr(g_Screen.m_fonts.m_pNormalFont, "          ");
}

void screenDestroy(void) {
    viewDestroy(g_Screen.m_pView); // This will also destroy all associated viewports and viewport managers

    bitmapDestroy(g_Screen.m_panels.m_pMainPanelBackground);
    bitmapDestroy(g_Screen.m_panels.s_pTopPanelBackground);
    bitmapDestroy(g_Screen.m_pIcons);
    bitmapDestroy(g_Screen.m_map.m_pTilemap);

    fontDestroyTextBitMap(g_Screen.m_panels.m_pGoldTextBitmap);
    fontDestroyTextBitMap(g_Screen.m_panels.m_pLumberTextBitmap);
    fontDestroyTextBitMap(g_Screen.m_panels.m_pUnitNameBitmap);
    fontDestroy(g_Screen.m_fonts.m_pNormalFont);
}

void gameGsCreate(void) {
    viewLoad(0);

    screenInit();

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

    unitManagerInitialize();
    buildingManagerInitialize();

    // load map file
    loadMap(g_Map.m_pName, s_copOffsets.mapbufCoplistStart, s_copOffsets.mapColorsCoplistStart);

    loadUi(s_copOffsets.topPanelColorsPos, s_copOffsets.panelColorsPos, s_copOffsets.simplePosTop, s_copOffsets.simplePosBottom);

    viewLoad(g_Screen.m_pView);

    systemSetDmaMask(DMAF_SPRITE, 1);

    systemUnuse();
}

/**
 * Screen coordinates to pathmap coordinates
 */
static inline tUbCoordYX screenPosToTile(tUwCoordYX pos) {
    UWORD uwY = pos.uwY - TOP_PANEL_HEIGHT;
    UWORD uwX = pos.uwX;
    // on map?
    if (uwY < MAP_HEIGHT) {
        tUwCoordYX mapOffset = g_Screen.m_map.m_pCamera->uPos;
        uwY += mapOffset.uwY;
        uwX += mapOffset.uwX;
        return (tUbCoordYX){.ubX = uwX / PATHMAP_TILE_SIZE, .ubY = uwY / PATHMAP_TILE_SIZE};
    }
    // on minimap?
    uwY -= (MAP_HEIGHT + MINIMAP_OFFSET_Y);
    if (uwY < PATHMAP_SIZE && ((uwX -= MINIMAP_OFFSET_X) < PATHMAP_SIZE)) {
        return (tUbCoordYX){.ubX = uwX, .ubY = uwY};
    }
    return (tUbCoordYX){.uwYX = -1};
}

static inline tUbCoordYX mapPosToTile(tUwCoordYX pos) {
    return (tUbCoordYX){.ubX = pos.uwX / PATHMAP_TILE_SIZE, .ubY = pos.uwY / PATHMAP_TILE_SIZE};
}

void drawSelectionIcon(IconIdx unitIcon, UBYTE idx) {
    tIcon *icon = &g_Screen.m_pSelectionIcons[idx];
    iconSetSource(icon, g_Screen.m_pIcons, unitIcon);
    iconDraw(icon, idx);
}

void drawSelectionHealth(UBYTE idx) {
    tIcon *icon = &g_Screen.m_pSelectionIcons[idx];
    // TODO: move into icons.c
    if (icon->healthValue) {
        // XXX: all hardcoded, using last pixel row in icon for health
        // TODO: optimize!
        UBYTE *pHpBar1 = icon->iconDstPtr +
            16 * 320 / 8 * BPP + // 16 pixels down
            1 * 320 / 8; // 1 palette bit down, so we draw on 0b00X0 (dark green)
        UBYTE *pHpBar2 = icon->iconDstPtr +
            17 * 320 / 8 * BPP + // 17 pixels down
            1 * 320 / 8; // 1 palette bit down, so we draw on 0b00X0 (dark green)
        UBYTE pixels = *icon->healthValue >> icon->healthShift;
        switch (icon->healthBase) {
            case HP_20:
                pixels = pixels + (pixels >> 1);
                break;
            case HP_25:
                pixels = pixels + (pixels >> 2);
                break;
        }
        ULONG value = 0;
        for (UBYTE p = 0; p < pixels; ++p) {
            value = (value >> 1) | 0x80000000L;
        }
        value >>= 1;
        *((ULONG*) pHpBar1) = value;
        *((ULONG*) pHpBar2) = value;
    }
}

void drawUnitInfo(Unit *unit) {
    fontFillTextBitMap(g_Screen.m_fonts.m_pNormalFont, g_Screen.m_panels.m_pUnitNameBitmap, UnitTypes[unit->type].name);
    fontDrawTextBitMap(g_Screen.m_panels.m_pMainPanelBuffer->pBack, g_Screen.m_panels.m_pUnitNameBitmap, 113, 24, 1, 0);
}

void drawBuildingInfo(Building *building) {
    if (building->action.action == ActionTrain) {
        // this must come first, since we want to re-use the blitter register values
        // from drawing the building icon
        UnitType *type1 = &UnitTypes[building->action.train.u5UnitType1];
        drawSelectionIcon(type1->iconIdx, 3);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
        iconSetHealth(&g_Screen.m_pSelectionIcons[3], &building->action.train.uwTimeLeft, type1->costs.timeShift, type1->costs.timeBase);
#pragma GCC diagnostic pop
        drawSelectionIcon(UnitTypes[building->action.train.u5UnitType2].iconIdx, 4);
        drawSelectionIcon(UnitTypes[building->action.train.u5UnitType3].iconIdx, 5);
    }
    fontFillTextBitMap(g_Screen.m_fonts.m_pNormalFont, g_Screen.m_panels.m_pUnitNameBitmap, BuildingTypes[building->type].name);
    fontDrawTextBitMap(g_Screen.m_panels.m_pMainPanelBuffer->pBack, g_Screen.m_panels.m_pUnitNameBitmap, 113, 24, 1, 0);
}

void drawInfoPanel(void) {
    if (g_Screen.m_ubBottomPanelDirty) {
        // TODO: only store and redraw the dirty part?
        g_Screen.m_ubBottomPanelDirty = 0;
        UBYTE idx = 0;
        if (g_Screen.m_ubSelectedUnitCount) {
            UnitTypeIndex type;
            for(; idx < g_Screen.m_ubSelectedUnitCount; ++idx) {
                Unit *unit = g_Screen.m_pSelectedUnit[idx];
                type = unit->type;
                UnitType *typePtr = &UnitTypes[type];
                drawSelectionIcon(typePtr->iconIdx, idx);
                iconSetHealth(&g_Screen.m_pSelectionIcons[idx], &unit->stats.hp, typePtr->stats.hpShift, typePtr->stats.hpBase);
            }
            for (UBYTE icon = idx; icon < NUM_SELECTION; ++icon) {
                drawSelectionIcon(ICON_NONE, icon);
            }
            iconSetSource(&g_Screen.m_pActionIcons[0], g_Screen.m_pIcons, ICON_MOVE);
            iconSetSource(&g_Screen.m_pActionIcons[1], g_Screen.m_pIcons, ICON_STOP);
            iconSetSource(&g_Screen.m_pActionIcons[2], g_Screen.m_pIcons, ICON_ATTACK);
            iconSetUnitAction(&g_Screen.m_pActionIcons[0], &iconActionMove);
            iconSetUnitAction(&g_Screen.m_pActionIcons[1], &iconActionStop);
            iconSetUnitAction(&g_Screen.m_pActionIcons[2], &iconActionAttack);
            if (idx == 1) {
                IconDefinitions *def = &g_UnitIconDefinitions[type];
                drawUnitInfo(g_Screen.m_pSelectedUnit[0]);
                for (UBYTE i = 0; i < 3; ++i) {
                    IconIdx idx = def->icons[i];
                    if (idx) {
                        iconSetSource(&g_Screen.m_pActionIcons[3 + i], g_Screen.m_pIcons, idx);
                        iconSetUnitAction(&g_Screen.m_pActionIcons[3 + i], def->unitActions[i]);
                    } else {
                        iconSetSource(&g_Screen.m_pActionIcons[3 + i], g_Screen.m_pIcons, ICON_FRAME);
                    }
                }
            } else {
                iconSetSource(&g_Screen.m_pActionIcons[3], g_Screen.m_pIcons, ICON_FRAME);
                iconSetSource(&g_Screen.m_pActionIcons[4], g_Screen.m_pIcons, ICON_FRAME);
                iconSetSource(&g_Screen.m_pActionIcons[5], g_Screen.m_pIcons, ICON_FRAME);
            }
        } else if (g_Screen.m_pSelectedBuilding) {
            Building *b = g_Screen.m_pSelectedBuilding;
            BuildingType *type = &BuildingTypes[b->type];
            drawSelectionIcon(type->iconIdx, 0);
            iconSetHealth(&g_Screen.m_pSelectionIcons[0], &b->hp, type->stats.hpShift, type->stats.baseHp);
            for (UBYTE icon = 1; icon < NUM_SELECTION; ++icon) {
                drawSelectionIcon(ICON_NONE, icon);
                }
            drawBuildingInfo(g_Screen.m_pSelectedBuilding);
            if (g_Screen.m_pSelectedBuilding->action.action == ActionBeingBuilt) {
                for (UBYTE icon = 0; icon < NUM_ACTION_ICONS - 1; ++icon) {
                    iconSetSource(&g_Screen.m_pActionIcons[icon], g_Screen.m_pIcons, ICON_FRAME);
                }
                iconSetSource(&g_Screen.m_pActionIcons[NUM_ACTION_ICONS - 1], g_Screen.m_pIcons, ICON_CANCEL);
                iconSetBuildingAction(&g_Screen.m_pActionIcons[NUM_ACTION_ICONS - 1], &iconActionCancelBuild);
            } else {
                IconDefinitions *def = &g_BuildingIconDefinitions[g_Screen.m_pSelectedBuilding->type];
                for (UBYTE i = 0; i < NUM_ACTION_ICONS; ++i) {
                    IconIdx idx = def->icons[i];
                    if (idx) {
                        iconSetSource(&g_Screen.m_pActionIcons[i], g_Screen.m_pIcons, idx);
                        iconSetBuildingAction(&g_Screen.m_pActionIcons[i], def->buildingActions[i]);
                    } else {
                        iconSetSource(&g_Screen.m_pActionIcons[i], g_Screen.m_pIcons, ICON_FRAME);
                    }
                }
            }
        } else {
            for (UBYTE icon = 0; icon < NUM_ACTION_ICONS; ++icon) {
                iconSetSource(&g_Screen.m_pActionIcons[icon], g_Screen.m_pIcons, ICON_FRAME);
            }
            for (UBYTE icon = 0; icon < NUM_SELECTION; ++icon) {
                drawSelectionIcon(ICON_NONE, icon);
            }
        }

        for (UBYTE icon = 0; icon < NUM_ACTION_ICONS; ++icon) {
            iconDraw(&g_Screen.m_pActionIcons[icon], icon);
        }
    } else if ((GameCycle & 16) != 16) {
        return;
    }
    // redraw health bars
    for (UBYTE icon = 0; icon < NUM_SELECTION; ++icon) {
        drawSelectionHealth(icon);
    }
}

void drawSelectionRubberBand(void) {
    UWORD x1 = s_selectionRubberBand.uwX;
    if (x1 == (UWORD)-1) {
        return;
    }
    UWORD uwHeight = s_selectionRubberBand.uwHeight;
    if (!uwHeight) {
        return;
    }
    UWORD uwWidthInWords = s_selectionRubberBand.uwWidth / 16;
    if (!uwWidthInWords) {
        return;
    }
    UWORD y1 = s_selectionRubberBand.uwY;
    UWORD uwModulo = (MAP_BUFFER_WIDTH_BYTES * uwHeight * BPP) - (uwWidthInWords * 2);
    UWORD uwBltSize = uwWidthInWords;
    if (y1 + uwHeight > MAP_HEIGHT + TOP_PANEL_HEIGHT) {
        uwBltSize |= (1 << 6);
    } else {
        uwBltSize |= (2 << 6);
    }
    PLANEPTR pMap = g_Screen.m_map.m_pBuffer->pBack->Planes[1] + (MAP_BUFFER_WIDTH_BYTES * y1 * BPP) + (x1 / 8);
    blitWait();
	g_pCustom->bltcon0 = USEA|USED|0x0F; // MINTERM_NOTA;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = uwModulo;
	g_pCustom->bltdmod = uwModulo;
    g_pCustom->bltdpt = pMap;
    g_pCustom->bltapt = pMap;
    g_pCustom->bltsize = uwBltSize;
}

void updateSelectionRubberBand(WORD x1, WORD y1, WORD x2, WORD y2) {
    if (x1 < 0) {
        s_selectionRubberBand.uwX = -1;
        return;
    }
    s_selectionRubberBand.uwX = MIN(x1, x2);
    s_selectionRubberBand.uwY = MIN(y1, y2) - TOP_PANEL_HEIGHT + g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY;
    s_selectionRubberBand.uwWidth = ABS(x2 - x1);
    s_selectionRubberBand.uwHeight = ABS(y2 - y1);
}

void drawSelectionRectangles(void) {
    for(UBYTE idx = 0; idx < NUM_SELECTION; ++idx) {
        Unit *unit = g_Screen.m_pSelectedUnit[idx];
        if (unit && idx < g_Screen.m_ubSelectedUnitCount) {
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
    UBYTE ubStartX = g_Screen.m_map.m_pCamera->uPos.uwX >> TILE_SHIFT;
	UBYTE ubStartY = g_Screen.m_map.m_pCamera->uPos.uwY >> TILE_SHIFT;
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
        pDstPlane += TILE_SIZE_BYTES;
    }
    systemSetDmaBit(DMAB_BLITHOG, 0);
}

void logicLoop(void) {
    GameCycle++;
}

void minimapUpdate(void) {
    if (GameCycle && (GameCycle & 127) != 127) {
        return;
    }
    UBYTE *pMinimap = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[0] + (MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO);
    UBYTE *pMinimap2 = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[1] + (MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO);
    UBYTE *pMinimap3 = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[2] + (MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO);
    for (UBYTE y = 0; y < MAP_SIZE * 2; ++y) {
        for (UBYTE x = 0; x < MAP_SIZE * 2;) {
            UBYTE minimapStatePixels = 0;
            UBYTE minimapStatePixels2 = 0;
            UBYTE minimapStatePixels3 = 0;
            for (UBYTE bit = 0; bit < 8; ++bit) {
                minimapStatePixels <<= 1;
                minimapStatePixels2 <<= 1;
                minimapStatePixels3 <<= 1;
                UWORD tile = g_Map.m_ubPathmapXY[x][y];
                switch (tile) {
                    case MAP_GROUND_FLAG: // 0b0011 -> light green
                        minimapStatePixels  |= 0b1;
                        minimapStatePixels2 |= 0b1;
                        minimapStatePixels3 |= 0b0;
                        break;
                    case MAP_GROUND_FLAG | MAP_COAST_FLAG: // 0b0100 -> lightBrown
                        minimapStatePixels  |= 0b0;
                        minimapStatePixels2 |= 0b0;
                        minimapStatePixels3 |= 0b1;
                        break;
                    case MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG: // 0b0010 -> dark green
                        minimapStatePixels  |= 0b0;
                        minimapStatePixels2 |= 0b1;
                        minimapStatePixels3 |= 0b0;
                        break;
                    case MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG: // 0b0111 -> lightBlue
                        minimapStatePixels  |= 0b1;
                        minimapStatePixels2 |= 0b1;
                        minimapStatePixels3 |= 0b1;
                        break;
                    case MAP_GROUND_FLAG | MAP_UNWALKABLE_FLAG:
                    case MAP_GROUND_FLAG | MAP_COAST_FLAG | MAP_UNWALKABLE_FLAG:
                    case MAP_UNWALKABLE_FLAG: // 0b0001 -> white
                        minimapStatePixels  |= 0b1;
                        minimapStatePixels2 |= 0b0;
                        minimapStatePixels3 |= 0b0;
                    
                }
                x++;
            }
            *pMinimap = minimapStatePixels;
            *pMinimap2 = minimapStatePixels2;
            *pMinimap3 = minimapStatePixels3;
            ++pMinimap;
            ++pMinimap2;
            ++pMinimap3;
        }
        pMinimap += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
        pMinimap2 += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
        pMinimap3 += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
    }
}

void handleLeftMouseUp(tUwCoordYX lmbDown, tUwCoordYX mousePos) {
    // TODO: extract constants
    if (lmbDown.uwY >= TOP_PANEL_HEIGHT + MAP_HEIGHT + MINIMAP_OFFSET_Y) {
        // Bottom panel
        if (lmbDown.uwX >= 211) {
            // Action icon area
            UWORD line = lmbDown.uwY - (TOP_PANEL_HEIGHT + MAP_HEIGHT + 24);
            if (line <= 18) {
                line = 0;
            } else if (line >= 23 && line <= 23 + 18) {
                line = 1;
            } else {
                return;
            }
            UWORD column = lmbDown.uwX - 211;
            if (column <= 26) {
                column = 0;
            } else if (column >= (26 + 7) && column <= (26 + 7) + 26) {
                column = 1;
            } else if (column >= (26 + 7) * 2 && column <= (26 + 7) * 2 + 26) {
                column = 2;
            } else {
                return;
            }
            if (g_Screen.m_ubSelectedUnitCount) {
                tIconActionUnit action = g_Screen.m_pActionIcons[column + line * 3].unitAction;
                if (action) {
                    iconRectSpritesUpdate(
                        211 + column * 33,
                        TOP_PANEL_HEIGHT + MAP_HEIGHT + 24 + line * 22);
                    action(g_Screen.m_pSelectedUnit, g_Screen.m_ubSelectedUnitCount);
                }
            } else if (g_Screen.m_pSelectedBuilding) {
                tIconActionBuilding action = g_Screen.m_pActionIcons[column + line * 3].buildingAction;
                if (action) {
                    iconRectSpritesUpdate(
                        211 + column * 33,
                        TOP_PANEL_HEIGHT + MAP_HEIGHT + 24 + line * 22);
                    action(g_Screen.m_pSelectedBuilding);
                }
            }
        } else if (lmbDown.uwX <= MINIMAP_OFFSET_X + 64) {
            UWORD y = lmbDown.uwY - (TOP_PANEL_HEIGHT + MAP_HEIGHT + MINIMAP_OFFSET_Y);
            if (y < 64) {
                UWORD x = lmbDown.uwX - MINIMAP_OFFSET_X;
                if (x < 64) {
                    cameraSetCoord(g_Screen.m_map.m_pCamera,
                            CLAMP((x - VISIBLE_TILES_X) / 2, 0, MAP_SIZE - VISIBLE_TILES_X) * TILE_SIZE,
                            CLAMP((y - VISIBLE_TILES_Y) / 2, 0, MAP_SIZE - VISIBLE_TILES_Y) * TILE_SIZE);
                    g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
                    g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
                }
            }
        }
        // TODO other areas
        return;
    }
    
    // menu button
    if (lmbDown.uwY <= TOP_PANEL_HEIGHT && lmbDown.uwX <= 60) {
        // TODO: menu. also extract constant here and in drawMenuButton
        return;
    }

    if (g_Screen.lmbAction) {
        tUbCoordYX tile = screenPosToTile(lmbDown);
        if (tile.ubX >= PATHMAP_SIZE) {
            return;
        }
        g_Screen.lmbAction(g_Screen.m_pSelectedUnit, g_Screen.m_ubSelectedUnitCount, tile);
        return;
    }

    // If we're on the map, select units
    tUbCoordYX tile1 = screenPosToTile(mousePos);
    tUbCoordYX tile2 = screenPosToTile(lmbDown);
    if (tile1.ubX >= PATHMAP_SIZE || tile2.ubX >= PATHMAP_SIZE) {
        return;
    }
    UBYTE x1 = MIN(tile1.ubX, tile2.ubX);
    UBYTE x2 = MAX(tile1.ubX, tile2.ubX);
    UBYTE y1 = MIN(tile1.ubY, tile1.ubY);
    UBYTE y2 = MAX(tile1.ubY, tile2.ubY);
    if (!(keyCheck(KEY_LSHIFT) || keyCheck(KEY_RSHIFT))) {
        g_Screen.m_ubSelectedUnitCount = 0;
        g_Screen.m_pSelectedBuilding = NULL;
        g_Screen.m_ubBottomPanelDirty = 1;
    }
    while (y1 <= y2) {
        UBYTE xcur = x1;    
        while (xcur <= x2) {
            tUbCoordYX tile = (tUbCoordYX){.ubX = xcur, .ubY = y1};
            Unit *unit = unitManagerUnitAt(tile);
            if (unit) {
                for(UBYTE idx = 0; idx < g_Screen.m_ubSelectedUnitCount; ++idx) {
                    if (g_Screen.m_pSelectedUnit[idx] == unit) {
                        goto outer;
                    }
                }
                g_Screen.m_pSelectedUnit[g_Screen.m_ubSelectedUnitCount++] = unit;
                g_Screen.m_ubBottomPanelDirty = 1;
                if (g_Screen.m_ubSelectedUnitCount == NUM_SELECTION) {
                    return;
                }
            }
            outer:;
            ++xcur;
        }
        ++y1;
    }
    if (!g_Screen.m_ubSelectedUnitCount) {
        // try selecting a building
        Building *building = buildingManagerBuildingAt(tile1);
        if (building) {
            g_Screen.m_pSelectedBuilding = building;
            g_Screen.m_ubBottomPanelDirty = 1;
        }
    }
}

void handleInput() {
    s_mouseX = mouseGetX(MOUSE_PORT_1);
    s_mouseY = mouseGetY(MOUSE_PORT_1);
    static tUwCoordYX lmbDown = {.ulYX = 0};
    
    tUwCoordYX mousePos = {.uwX = s_mouseX, .uwY = s_mouseY};

    if (s_Mode == edit) {
        if (keyCheck(KEY_RBRACKET) && !(GameCycle % 4)) {
            SelectedTile++;
            // XXX: 4-tile buildings
            if (SelectedTile >= 24 && SelectedTile < 48 && SelectedTile % 4) {
                SelectedTile += 3;
            }
        } else if (keyCheck(KEY_LBRACKET) && !(GameCycle % 4)) {
            SelectedTile--;
            // XXX: 4-tile buildings
            if (SelectedTile >= 24 && SelectedTile < 48 && SelectedTile % 4) {
                SelectedTile -= 3;
            }
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
            tUbCoordYX tile = screenPosToTile(mousePos);
            if (tile.ubX >= PATHMAP_SIZE) {
                return;
            }
            UBYTE ubX = tile.ubX / TILE_SIZE_FACTOR;
            UBYTE ubY = tile.ubY / TILE_SIZE_FACTOR;
            g_Map.m_ulTilemapXY[ubX][ubY] = (ULONG)g_Screen.m_cursorBobs.pFirstTile;
            // XXX: 4-tile buildings
            if (SelectedTile >= 24 && SelectedTile < 48) {
                g_Map.m_ulTilemapXY[ubX + 1][ubY] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES;
                g_Map.m_ulTilemapXY[ubX][ubY + 1] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES * 2;
                g_Map.m_ulTilemapXY[ubX + 1][ubY + 1] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES * 3;
            }
        }
        // XXX: 4-tile buildings
        if (s_Mode == game) {
            g_Screen.m_cursorBobs.ubCount = 0;
        } else if (SelectedTile >= 24 && SelectedTile < 48) {
            SelectedTile = SelectedTile / 4 * 4;
            g_Screen.m_cursorBobs.ubCount = 4;
        } else {
            g_Screen.m_cursorBobs.ubCount = 1;
        }
        g_Screen.m_cursorBobs.pFirstTile = g_Screen.m_map.m_pTilemap->Planes[0] + (SelectedTile * TILE_FRAME_BYTES);
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
    } else if (mousePos.uwY >= TOP_PANEL_HEIGHT + MAP_HEIGHT + BOTTOM_PANEL_HEIGHT) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, 0, CAMERA_MOVE_DELTA);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY = g_Screen.m_map.m_pCamera->uPos.uwY % TILE_SIZE;
    }
    if (mousePos.uwX <= 1) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, -CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    } else if (mousePos.uwX >= MAP_WIDTH - 1) {
        cameraMoveBy(g_Screen.m_map.m_pCamera, CAMERA_MOVE_DELTA, 0);
        g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX = g_Screen.m_map.m_pCamera->uPos.uwX % TILE_SIZE;
    }

    if (mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
        if (lmbDown.uwY) {
            if (lmbDown.uwY - mousePos.uwY) {
                updateSelectionRubberBand(lmbDown.uwX, lmbDown.uwY, mousePos.uwX, mousePos.uwY);
            }
        } else {
            lmbDown = mousePos;
            updateSelectionRubberBand(-1, -1, -1, -1);
        }
    } else if (lmbDown.ulYX) {
        handleLeftMouseUp(mousePos, lmbDown);
        lmbDown.ulYX = 0;
        updateSelectionRubberBand(-1, -1, -1, -1);
    } else if (g_Screen.m_ubSelectedUnitCount && mouseCheck(MOUSE_PORT_1, MOUSE_RMB)) {
        tUbCoordYX tile = screenPosToTile(mousePos);
        if (tile.ubX >= PATHMAP_SIZE) {
            return;
        }
        for(UBYTE idx = 0; idx < g_Screen.m_ubSelectedUnitCount; ++idx) {
            Unit *unit = g_Screen.m_pSelectedUnit[idx];
            if (unit) {
                actionMoveTo(unit, tile);
            } else {
                break;
            }
        }
    }
}

void colorCycle(void) {
    // if ((GameCycle & 31) == 31) {
    //     if (GameCycle & 32) {
    //         // XXX: hardcoded gpl indices
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
    //     } else if (GameCycle & 64) {
    //         // XXX: hardcoded gpl indices
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
    //     } else {
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[12]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColorsCoplistStart];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[12]);
    //     }
    // }
}

void fowUpdate(void) {
}

void drawResources(void) {
    if (!g_Screen.m_ubTopPanelDirty && (GameCycle & 63) != 63) {
        return;
    }
    static UWORD gold = -1;
    static UWORD lumber = -1;
    static char str[RESOURCE_DIGITS + 1] = {'\0'};
    if (!GameCycle || (GameCycle & 64 && gold != g_pPlayers[0].gold)) {
        gold = g_pPlayers[0].gold;
        snprintf(str, RESOURCE_DIGITS, "%d", gold);
        fontFillTextBitMap(g_Screen.m_fonts.m_pNormalFont, g_Screen.m_panels.m_pGoldTextBitmap, str);
        fontDrawTextBitMap(g_Screen.m_panels.m_pTopPanelBuffer->pBack, g_Screen.m_panels.m_pGoldTextBitmap, 128, 0, 1, FONT_RIGHT);
    }
    if (!GameCycle || (!(GameCycle & 64) && lumber != g_pPlayers[0].lumber)) {
        lumber = g_pPlayers[0].lumber;
        snprintf(str, RESOURCE_DIGITS, "%d", lumber);
        fontFillTextBitMap(g_Screen.m_fonts.m_pNormalFont, g_Screen.m_panels.m_pLumberTextBitmap, str);
        fontDrawTextBitMap(g_Screen.m_panels.m_pTopPanelBuffer->pBack, g_Screen.m_panels.m_pLumberTextBitmap, 232, 0, 1, FONT_RIGHT);
    }
}

void drawMissiles(void) {
}

void drawUnits(void) {
    // process all units
    tUbCoordYX tileTopLeft = mapPosToTile(g_Screen.m_map.m_pCamera->uPos);
    unitManagerProcessUnits(
        tileTopLeft,
        (tUbCoordYX){.ubX = (tileTopLeft.ubX + VISIBLE_TILES_X * 2), .ubY = (tileTopLeft.ubY + VISIBLE_TILES_Y * 2)}
    );
    buildingManagerProcess();
}

void drawFog(void) {
}

void drawMap(void) {
    drawAllTiles();
    drawUnits();
    drawMissiles();
    drawFog();
}

void drawCursorBobs(void) {
    UBYTE cnt = g_Screen.m_cursorBobs.ubCount;
    if (cnt) {
        UWORD uwMouseY = s_mouseY - TOP_PANEL_HEIGHT;
        if (uwMouseY >= MAP_HEIGHT) {
            return;
        }
        // setup tile drawing, all of these should be compiled to immediate operations, they
        // only use constants
	    WORD wDstModulo = MAP_BUFFER_WIDTH_BYTES - TILE_SIZE_BYTES; // same as bitmapGetByteWidth(g_Screen.m_map.m_pBuffer->pBack) - TILE_SIZE_BYTES;
        WORD wSrcModulo = 0;
        UWORD uwBlitWords = TILE_SIZE_WORDS;
        UWORD uwHeight = TILE_SIZE * BPP;
        UWORD uwBltCon0 = USEA|USED|MINTERM_A;
        UWORD uwBltsize = (uwHeight << 6) | uwBlitWords;

        // Figure out where to actually draw
        UBYTE ubStartX = (s_mouseX + g_Screen.m_map.m_pBuffer->pCamera->uPos.uwX) >> TILE_SHIFT;
        UBYTE ubStartY = (uwMouseY + g_Screen.m_map.m_pBuffer->pCamera->uPos.uwY) >> TILE_SHIFT;
        if (cnt > 1 && (ubStartX >= VISIBLE_TILES_X - 1 || ubStartY >= VISIBLE_TILES_Y - 1)) {
            return;
        }

        // Get pointer to start of drawing area
        UBYTE *pDstPlane = g_Screen.m_map.m_pBuffer->pBack->Planes[0];
        pDstPlane += ubStartY * TILE_SIZE * MAP_BUFFER_WIDTH_BYTES * BPP;
        pDstPlane += ubStartX * TILE_SIZE_BYTES;

        // setup blitter registers that won't change
        systemSetDmaBit(DMAB_BLITHOG, 1);
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xFFFF;
        g_pCustom->bltalwm = 0xFFFF;
        g_pCustom->bltamod = wSrcModulo;
        g_pCustom->bltdmod = wDstModulo;

        UBYTE *pSrc = g_Screen.m_cursorBobs.pFirstTile;
        if (cnt == 1) {
            g_pCustom->bltdpt = pDstPlane;
            g_pCustom->bltapt = (APTR)pSrc;
            g_pCustom->bltsize = uwBltsize;
        } else if (cnt == 4) {
            g_pCustom->bltdpt = pDstPlane;
            g_pCustom->bltapt = (APTR)pSrc;
            g_pCustom->bltsize = uwBltsize;
            blitWait();
            g_pCustom->bltapt = (APTR)(pSrc + TILE_FRAME_BYTES * 2);
            g_pCustom->bltsize = uwBltsize;
            blitWait();
            g_pCustom->bltdpt = pDstPlane + TILE_SIZE_BYTES;
            g_pCustom->bltapt = (APTR)(pSrc + TILE_FRAME_BYTES);
            g_pCustom->bltsize = uwBltsize;
            blitWait();
            g_pCustom->bltapt = (APTR)(pSrc + TILE_FRAME_BYTES * 3);
            g_pCustom->bltsize = uwBltsize;
        }
    }
}

void drawMessages(void) {
}

void drawActionButtons(void) {
}

void drawMenuButton(void) {
    static UWORD topHighlight = 0;
    static UBYTE blue1 = 7; static UWORD red1 = 0x724;
    static UBYTE blue2 = 8; static UWORD red2 = 0xa44;
    static UBYTE blue3 = 9; static UWORD red3 = 0xd45;
    if (s_mouseY < TOP_PANEL_HEIGHT && s_mouseX <= 60) {
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
}

void drawMinimapRectangle(void) {
    minimapSpritesUpdate(
        g_Screen.m_map.m_pCamera->uPos.uwX / PATHMAP_TILE_SIZE + MINIMAP_OFFSET_X,
        g_Screen.m_map.m_pCamera->uPos.uwY / PATHMAP_TILE_SIZE + TOP_PANEL_HEIGHT + MAP_HEIGHT + MINIMAP_OFFSET_Y);
}

void drawStatusLine(void) {
}

void updateDisplay(void) {
    drawMap();
    drawCursorBobs();
    drawMessages();
    drawMenuButton();
    drawActionButtons();
    drawInfoPanel();
    drawResources();
    drawStatusLine();
    drawSelectionRubberBand();

    // finish frame
    viewProcessManagers(g_Screen.m_pView);
    copSwapBuffers();
    vPortWaitUntilEnd(g_Screen.m_panels.m_pMainPanel);

    // update sprites during vblank
    mouseSpriteUpdate(s_mouseX, s_mouseY);
    drawSelectionRectangles();
    drawMinimapRectangle();
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

    screenDestroy();
    unitManagerDestroy();
    buildingManagerDestroy();
}
