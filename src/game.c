#include <limits.h>

#include "include/main.h"
#include "include/game.h"
#include "include/utils.h"
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

static tState *s_pNewState;

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
    UWORD mouseSprite;
    UWORD selectionSprites;
    UWORD topPanelBitplanes;
    UWORD topPanelColors;
    UWORD mapBitplanes;
    UWORD mapColors;
    UWORD bottomPanelBitplanes;
    UWORD bottomPanelColors;
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

void loadMap(const char* name, UWORD mapBitplanes, UWORD mapColors) {
    char* mapname = MAPDIR LONGEST_MAPNAME;

    snprintf(mapname + strlen(MAPDIR), strlen(LONGEST_MAPNAME) + 1, "%s.map", name);
    tFile *map = diskFileOpen(mapname, "r");
    assert(map, "ERROR: Cannot open file %s!\n", mapname);

    mapLoad(map);
    playersLoad(map);
    unitsLoad(map);
    buildingsLoad(map);
    fileClose(map);

    // now setup map viewport
    char* palname = PALDIR "for.plt";
    strncpy(palname + strlen(PALDIR), g_Map.m_pTileset + strlen(TILESETDIR), 3);
    paletteLoadFromPath(palname, g_Screen.m_map.m_pPalette, COLORS + 1);
    tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[mapColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[mapColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_map.m_pPalette[i]);
    }
    g_Screen.m_map.m_pBuffer = simpleBufferCreate(0,
                                    TAG_SIMPLEBUFFER_VPORT, g_Screen.m_map.m_pVPort,
                                    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                    TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_BUFFER_WIDTH,
                                    TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_BUFFER_HEIGHT,
                                    TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                    TAG_SIMPLEBUFFER_COPLIST_OFFSET, mapBitplanes,
                                    TAG_SIMPLEBUFFER_USE_X_SCROLLING, 1,
                                    TAG_END);
    g_Screen.m_map.m_pCamera = cameraCreate(g_Screen.m_map.m_pVPort, 0, 0, MAP_SIZE * TILE_SIZE, MAP_SIZE * TILE_SIZE, 0);
}

void loadUi(UWORD topPanelColors, UWORD bottomPanelColors, UWORD topPanelBitplanes, UWORD bottomPanelBitplanes) {
    // create panel area
    paletteLoadFromPath("resources/palettes/hgui.plt", g_Screen.m_panels.m_pPalette, COLORS);

    tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[topPanelColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[topPanelColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }

    pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[bottomPanelColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }
    pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[bottomPanelColors];
    for (UBYTE i = 0; i < COLORS; i++) {
        copSetMove(&pCmds[i].sMove, &g_pCustom->color[i], g_Screen.m_panels.m_pPalette[i]);
    }

    g_Screen.m_panels.m_pTopPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, g_Screen.m_panels.m_pTopPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, topPanelBitplanes,
                                        TAG_END);
    g_Screen.m_panels.s_pTopPanelBackground = bitmapCreateFromPath("resources/ui/toppanel.bm", 0);
    bitmapLoadFromPath(g_Screen.m_panels.m_pTopPanelBuffer->pFront, "resources/ui/toppanel.bm", 0, 0);

    g_Screen.m_panels.m_pMainPanelBuffer = simpleBufferCreate(0,
                                        TAG_SIMPLEBUFFER_VPORT, g_Screen.m_panels.m_pMainPanel,
                                        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
                                        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                        TAG_SIMPLEBUFFER_COPLIST_OFFSET, bottomPanelBitplanes,
                                        TAG_END);
    g_Screen.m_panels.m_pMainPanelBackground = bitmapCreateFromPath("resources/ui/bottompanel.bm", 0);
    bitmapLoadFromPath(g_Screen.m_panels.m_pMainPanelBuffer->pFront, "resources/ui/bottompanel.bm", 0, 0);

    g_Screen.m_pIcons = bitmapCreateFromPath("resources/ui/icons.bm", 0);
    iconInit(&g_Screen.m_pActionIcons[0], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_MOVE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 208, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[1], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_STOP, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 240, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[2], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_ATTACK, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 272, .uwY = 24});
    iconInit(&g_Screen.m_pActionIcons[3], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 208, .uwY = 46});
    iconInit(&g_Screen.m_pActionIcons[4], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 240, .uwY = 46});
    iconInit(&g_Screen.m_pActionIcons[5], 32, 18, 0xffff, 0xffff, g_Screen.m_pIcons, ICON_FRAME, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 272, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[0], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 80, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[1], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 112, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[2], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 144, .uwY = 24});
    iconInit(&g_Screen.m_pSelectionIcons[3], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 80, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[4], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 112, .uwY = 46});
    iconInit(&g_Screen.m_pSelectionIcons[5], 32, 18, 0x1fff, 0xfffc, g_Screen.m_pIcons, ICON_NONE, g_Screen.m_panels.m_pMainPanelBuffer->pFront, (tUwCoordYX){.uwX = 144, .uwY = 46});
}

void screenInit(void) {
    memset(&g_Screen, 0, sizeof(struct Screen));

    g_Screen.m_ubTopPanelDirty = 1;
    g_Screen.m_ubBottomPanelDirty = 1;

    g_Screen.m_fonts.m_pNormalFont = fontCreateFromPath("resources/ui/uni54.fnt");
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
    bitmapDestroy(g_Screen.m_map.m_pFogOfWarMask);

    fontDestroyTextBitMap(g_Screen.m_panels.m_pGoldTextBitmap);
    fontDestroyTextBitMap(g_Screen.m_panels.m_pLumberTextBitmap);
    fontDestroyTextBitMap(g_Screen.m_panels.m_pUnitNameBitmap);
    fontDestroy(g_Screen.m_fonts.m_pNormalFont);

    for (UBYTE i = 0; i < MSG_COUNT; ++i) {
        if (g_Screen.m_pMessageBitmaps[i]->pBitMap) {
            fontDestroyTextBitMap(g_Screen.m_pMessageBitmaps[i]);
        }
    }
}

void gameGsCreate(void) {
    s_pNewState = NULL;
    viewLoad(0);

    // mouse uses colors 17, 18
    g_pCustom->color[17] = 0x0ccc;
    g_pCustom->color[18] = 0x0888;
    // minimap rectangle is light gray, using color 1 of sprites 0 and 1 (17 & 21)
    g_pCustom->color[21] = 0x0ccc;
    // all the selection rects use the 4th color (glowing green)
    for (int i = 19; i < 32; i += 4) {
        g_pCustom->color[i] = 0x02f4;
    }

    // most everything else is a glowing green
    for (int i = 23; i < 32; ++i) {
        g_pCustom->color[i] = 0x02f4;
    }

    screenInit();

    // Calculate copperlist size
    s_copOffsets.mouseSprite = 0;
    s_copOffsets.selectionSprites = s_copOffsets.mouseSprite + mouseSpriteGetRawCopplistInstructionCountLength();
    s_copOffsets.topPanelBitplanes = s_copOffsets.selectionSprites + selectionSpritesGetRawCopplistInstructionCountLength();
    s_copOffsets.topPanelColors = s_copOffsets.topPanelBitplanes + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.mapBitplanes = s_copOffsets.topPanelColors + COLORS;
    s_copOffsets.mapColors = s_copOffsets.mapBitplanes + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.bottomPanelBitplanes = s_copOffsets.mapColors + COLORS;
    s_copOffsets.bottomPanelColors = s_copOffsets.bottomPanelBitplanes + simpleBufferGetRawCopperlistInstructionCount(BPP);
    s_copOffsets.copListLength = s_copOffsets.bottomPanelColors + COLORS;

    // Create the game view
    g_Screen.m_pView = viewCreate(0,
                         TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
                         TAG_VIEW_COPLIST_RAW_COUNT, s_copOffsets.copListLength,
                         TAG_VIEW_WINDOW_START_Y, SCREEN_PAL_YOFFSET,
                         TAG_VIEW_WINDOW_HEIGHT, TOP_PANEL_HEIGHT + MAP_HEIGHT + BOTTOM_PANEL_HEIGHT,
                         TAG_DONE);

    // setup mouse
    mouseSpriteSetup(g_Screen.m_pView, s_copOffsets.mouseSprite);

    // setup selection rectangles
    selectionSpritesSetup(g_Screen.m_pView, s_copOffsets.selectionSprites);

    createViewports();

    unitManagerInitialize();
    buildingManagerInitialize();

    // load map file
    loadMap(g_Map.m_pName, s_copOffsets.mapBitplanes, s_copOffsets.mapColors);

    loadUi(s_copOffsets.topPanelColors, s_copOffsets.bottomPanelColors, s_copOffsets.topPanelBitplanes, s_copOffsets.bottomPanelBitplanes);

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
    UWORD uwBltCon0Fog = USEA|USED|0b11000000; // use B as deleting mask
    UWORD uwBltCon0Dark = USED;
	UWORD uwBltsize = (uwHeight << 6) | uwBlitWords;

    static UWORD uwBdat = 0b1010101010101010;

    // Figure out which tiles to actually draw, depending on the
    // current camera position
    UBYTE ubStartX = g_Screen.m_map.m_pCamera->uPos.uwX >> TILE_SHIFT;
	UBYTE ubStartY = g_Screen.m_map.m_pCamera->uPos.uwY >> TILE_SHIFT;
	UBYTE ubEndX = ubStartX + (MAP_BUFFER_WIDTH >> TILE_SHIFT);

    // Get pointer to start of drawing area
    PLANEPTR pDstPlane = g_Screen.m_map.m_pBuffer->pBack->Planes[0];
    PLANEPTR pFogPlane = g_Screen.m_map.m_pFogOfWarMask->Planes[0];

    // setup blitter registers that won't change
    systemSetDmaBit(DMAB_BLITHOG, 1);
    blitWait();
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = wSrcModulo;
    g_pCustom->bltbmod = 0;
	g_pCustom->bltdmod = wDstModulo;
    g_pCustom->bltbpt = pFogPlane;
    g_pCustom->bltbdat = uwBdat;

    // draw as fast as we can
    UWORD uwBltCon0Used;
    for (UBYTE x = ubStartX; x < ubEndX; x++) {
        // manually unrolled loop to draw (MAP_BUFFER_HEIGHT / TILE_SIZE) tiles
        ULONG *pTileBitmapOffset = &(g_Map.m_ulVisibleMapXY[x][ubStartY]);
        APTR apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltdpt = pDstPlane;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        ++pTileBitmapOffset;
        apt = (APTR)*pTileBitmapOffset;
        uwBltCon0Used = apt ? (IS_TILE_UNCOVERED(apt) ? uwBltCon0 : uwBltCon0Fog) : uwBltCon0Dark;
        blitWait();
        g_pCustom->bltcon0 = uwBltCon0Used;
        g_pCustom->bltapt = apt;
        g_pCustom->bltsize = uwBltsize;
        pDstPlane += TILE_SIZE_BYTES;
    }
    systemSetDmaBit(DMAB_BLITHOG, 0);
}

void logicLoop(void) {
    GameCycle++;
}

void minimapUpdate(void) {
    static UBYTE y = 0;
    static UBYTE *pMinimapBitPlane1 = 0;
    static UBYTE *pMinimapBitPlane2 = 0;
    static UBYTE *pMinimapBitPlane3 = 0;
    y = y % PATHMAP_SIZE;
    if (y == 0) {
        pMinimapBitPlane1 = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[0] + ((MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO));
        pMinimapBitPlane2 = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[1] + ((MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO));
        pMinimapBitPlane3 = g_Screen.m_panels.m_pMainPanelBuffer->pBack->Planes[2] + ((MINIMAP_OFFSET_X / 8) + (MINIMAP_OFFSET_Y * MINIMAP_MODULO));
    }
    for (UBYTE x = 0; x < PATHMAP_SIZE;) {
        ULONG minimapStatePixels1 = 0;
        ULONG minimapStatePixels2 = 0;
        ULONG minimapStatePixels3 = 0;
        for (UBYTE bit = 0; bit < 32; bit += 2) {
            minimapStatePixels1 <<= 2;
            minimapStatePixels2 <<= 2;
            minimapStatePixels3 <<= 2;
            UWORD pathTile;
            UBYTE mapTileX = x / TILE_SIZE_FACTOR;
            ULONG mapTile = g_Map.m_ulVisibleMapXY[mapTileX][y / TILE_SIZE_FACTOR];
            if (IS_TILE_UNCOVERED(mapTile)) {
                UWORD *uwPathmap = (UWORD*)g_Map.m_ubPathmapYX[y];
                pathTile = uwPathmap[mapTileX];
            } else {
                if (mapTile) {
                    BYTE mapTileIndex = tileBitmapOffsetToTileIndex(mapTile);
                    if ((mapTileIndex -= 6) < 0) {
                        pathTile = MAP_GROUND_FLAG;
                    } else if ((mapTileIndex -= 3) < 0) {
                        pathTile = MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG;
                    } else if ((mapTileIndex -= 12) < 0) {
                        pathTile = MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG;
                    } else if ((mapTileIndex -= 29) < 0) {
                        // invisible building must belong to enemy
                        pathTile = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_OWNER_BIT;
                    } else {
                        pathTile = MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG;
                    }
                    pathTile = (pathTile << 8) | pathTile; // we process 2 tiles at once
                } else {
                    pathTile = 0;
                }
            }
            // buildings, water, forests, and coasts are always 2 path tiles wide, so we only need to check
            // repeated flags there
            switch (pathTile) {
                case ((MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG) << 8) |
                        MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_GOLDMINE_FLAG: // 0b0001 -> white
                    minimapStatePixels1 |= 0b11;
                    break;
                case ((MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG) << 8) |
                        MAP_FOREST_FLAG | MAP_UNWALKABLE_FLAG: // 0b0010 -> dark green
                    minimapStatePixels2 |= 0b11;
                    break;
                case (MAP_GROUND_FLAG << 8) |
                        MAP_GROUND_FLAG: // 0b0011 -> light green
                    minimapStatePixels1 |= 0b11;
                    minimapStatePixels2 |= 0b11;
                    break;

                case ((MAP_WATER_FLAG) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG: // 0b0100 -> lightBrown
                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG) << 8) |
                        MAP_WATER_FLAG: // 0b0100 -> lightBrown
                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG: // 0b0100 -> lightBrown
                    minimapStatePixels3 |= 0b11;
                    break;
                
                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG | MAP_UNWALKABLE_FLAG | MAP_OWNER_BIT:
                        // 0b0101 -> red, enemy unit right; 0b0100 -> lightBrown left
                    minimapStatePixels1 |= 0b01;
                    minimapStatePixels3 |= 0b11;
                    break;

                case (MAP_GROUND_FLAG << 8) |
                        MAP_GROUND_FLAG | MAP_UNWALKABLE_FLAG | MAP_OWNER_BIT:
                        // 0b0101 -> red, enemy unit right; 0b0011 -> lightGreen left
                    minimapStatePixels1 |= 0b11;
                    minimapStatePixels2 |= 0b10;
                    minimapStatePixels3 |= 0b01;
                    break;

                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG | MAP_UNWALKABLE_FLAG | MAP_OWNER_BIT) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG: // 0b0101 -> red, enemy unit left; 0b0100 -> lightBrown right
                    minimapStatePixels1 |= 0b10;
                    minimapStatePixels3 |= 0b11;
                    break;

                case ((MAP_GROUND_FLAG | MAP_UNWALKABLE_FLAG | MAP_OWNER_BIT) << 8) |
                        MAP_GROUND_FLAG: // 0b0101 -> red, enemy unit left; 0b0011 -> lightGreen right
                    minimapStatePixels1 |= 0b11;
                    minimapStatePixels2 |= 0b01;
                    minimapStatePixels3 |= 0b10;
                    break;

                case ((MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_OWNER_BIT) << 8) |
                        MAP_UNWALKABLE_FLAG | MAP_BUILDING_FLAG | MAP_OWNER_BIT: // 0b0101 -> red, enemy building
                    minimapStatePixels1 |= 0b11;
                    minimapStatePixels3 |= 0b11;
                    break;

                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG | MAP_UNWALKABLE_FLAG:
                        // 0b0110 -> dark blue, friendly right; 0b0100 -> lightBrown left
                    minimapStatePixels2 |= 0b01;
                    minimapStatePixels3 |= 0b11;
                    break;

                case (MAP_GROUND_FLAG << 8) |
                        MAP_GROUND_FLAG | MAP_UNWALKABLE_FLAG:
                        // 0b0110 -> dark blue, friendly right; 0b0011 -> lightGreen left
                    minimapStatePixels1 |= 0b10;
                    minimapStatePixels2 |= 0b11;
                    minimapStatePixels3 |= 0b01;
                    break;

                case ((MAP_GROUND_FLAG | MAP_COAST_FLAG | MAP_UNWALKABLE_FLAG) << 8) |
                        MAP_GROUND_FLAG | MAP_COAST_FLAG: // 0b0110 -> dark blue, friendly unit left; 0b0100 -> lightBrown right
                    minimapStatePixels2 |= 0b10;
                    minimapStatePixels3 |= 0b11;
                    break;

                case ((MAP_GROUND_FLAG | MAP_UNWALKABLE_FLAG) << 8) |
                        MAP_GROUND_FLAG: // 0b0110 -> dark blue, friendly unit left; 0b0011 -> lightGreen right
                    minimapStatePixels1 |= 0b01;
                    minimapStatePixels2 |= 0b11;
                    minimapStatePixels3 |= 0b10;
                    break;
                
                case ((MAP_BUILDING_FLAG | MAP_UNWALKABLE_FLAG) << 8) |
                        MAP_BUILDING_FLAG | MAP_UNWALKABLE_FLAG: // 0b0110 -> dark blue, friendly building
                    minimapStatePixels2 |= 0b11;
                    minimapStatePixels3 |= 0b11;
                    break;
                case ((MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG) << 8) |
                        MAP_WATER_FLAG | MAP_UNWALKABLE_FLAG: // 0b0111 -> lightBlue
                    minimapStatePixels1 |= 0b11;
                    minimapStatePixels2 |= 0b11;
                    minimapStatePixels3 |= 0b11;
                    break;
                default: // 0b0000 -> black
                    break;
            }
            x += 2;
        }
        memcpy(pMinimapBitPlane1, &minimapStatePixels1, 4);
        memcpy(pMinimapBitPlane2, &minimapStatePixels2, 4);
        memcpy(pMinimapBitPlane3, &minimapStatePixels3, 4);
        pMinimapBitPlane1 += 4;
        pMinimapBitPlane2 += 4;
        pMinimapBitPlane3 += 4;
    }
    pMinimapBitPlane1 += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
    pMinimapBitPlane2 += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
    pMinimapBitPlane3 += (MINIMAP_MODULO - (MINIMAP_WIDTH / 8));
    ++y;
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
            if (unit && unit->owner == g_ubThisPlayer) {
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

void showToolTip(void) {
    if (s_mouseY >= TOP_PANEL_HEIGHT + MAP_HEIGHT + MINIMAP_OFFSET_Y) {
        // Bottom panel
        if (s_mouseX >= 211) {
            // Action icon area
            UWORD line = s_mouseY - (TOP_PANEL_HEIGHT + MAP_HEIGHT + 24);
            if (line <= 18) {
                line = 0;
            } else if (line >= 23 && line <= 23 + 18) {
                line = 1;
            } else {
                return;
            }
            UWORD column = s_mouseX - 211;
            if (column <= 26) {
                column = 0;
            } else if (column >= (26 + 7) && column <= (26 + 7) + 26) {
                column = 1;
            } else if (column >= (26 + 7) * 2 && column <= (26 + 7) * 2 + 26) {
                column = 2;
            } else {
                return;
            }
            IconIdx icon = g_Screen.m_pActionIcons[column + line * 3].iconIdx;
            if (icon != ICON_NONE) {
                logMessage(icon + MSG_HOVER_ICON);
                return;
            }
        }
        // TODO other areas
        return;
    }
    
    // menu button
    if (s_mouseY <= TOP_PANEL_HEIGHT && s_mouseX <= 60) {
        // TODO: menu. also extract constant here and in drawMenuButton
        logMessage(MSG_MENU_BUTTON);
        return;
    }

    // If we're on the map, select units
    tUbCoordYX tile = screenPosToTile((tUwCoordYX){.uwX = s_mouseX, .uwY = s_mouseY});
    Unit *unit = unitManagerUnitAt(tile);
    if (unit) {
        logMessage(unit->type + MSG_HOVER_UNIT);
        return;
    } else {
        Building *b = buildingManagerBuildingAt(tile);
        if (b) {
            logMessage(b->type + MSG_HOVER_BUILDING);
            return;
        }
    }

    logMessage(MSG_COUNT);
}

void handleInput() {
    s_mouseX = mouseGetX(MOUSE_PORT_1);
    s_mouseY = mouseGetY(MOUSE_PORT_1);
    static tUwCoordYX lmbDown = {.ulYX = 0};
    static tUwCoordYX lastMousePosition = {.ulYX = 0};
    static UBYTE framesWithoutMouseMove = 0;

    tUwCoordYX mousePos = {.uwX = s_mouseX, .uwY = s_mouseY};
    if (lastMousePosition.ulYX != mousePos.ulYX) {
        lastMousePosition.ulYX = mousePos.ulYX;
        framesWithoutMouseMove = 0;
    } else {
        ++framesWithoutMouseMove;
    }

#ifdef ACE_DEBUG
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
            tFile *map = diskFileOpen(mapname, "w");
            assert(map, "ERROR: Cannot open file %s!\n", mapname);
            fileWrite(map, "for", 3);
            for (int x = 0; x < MAP_SIZE; x++) {
                // TODO: fix file writing
                // fileWrite(map, s_ulTilemap[x], MAP_SIZE);
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
            g_Map.m_ulVisibleMapXY[ubX][ubY] = (ULONG)g_Screen.m_cursorBobs.pFirstTile;
            g_Map.m_ubTilemapXY[ubX][ubY] = (ULONG)g_Screen.m_cursorBobs.ubFirstTile;
            // XXX: 4-tile buildings
            if (SelectedTile >= 24 && SelectedTile < 48) {
                g_Map.m_ulVisibleMapXY[ubX + 1][ubY] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES;
                g_Map.m_ubTilemapXY[ubX + 1][ubY] = g_Screen.m_cursorBobs.ubFirstTile + 1;
                g_Map.m_ulVisibleMapXY[ubX][ubY + 1] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES * 2;
                g_Map.m_ubTilemapXY[ubX][ubY + 1] = g_Screen.m_cursorBobs.ubFirstTile + 2;
                g_Map.m_ulVisibleMapXY[ubX + 1][ubY + 1] = (ULONG)g_Screen.m_cursorBobs.pFirstTile + TILE_FRAME_BYTES * 3;
                g_Map.m_ubTilemapXY[ubX + 1][ubY + 1] = g_Screen.m_cursorBobs.ubFirstTile + 3;
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
#endif

    if (keyCheck(KEY_ESCAPE)) {
        s_pNewState = g_pGameState;
#ifdef ACE_DEBUG
    } else if (keyCheck(KEY_C)) {
        copDumpBfr(g_Screen.m_pView->pCopList->pBackBfr);
    } else if (keyCheck(KEY_E)) {
        s_Mode = edit;  
#endif
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
        // find potential things to act on at the target location
        UBYTE mayBeHarvested = 0;
        Unit *unitTarget = NULL;
        Building *buildingTarget = NULL;
        if (mapIsUncovered(tile.ubX, tile.ubY)) {
            if (mapIsHarvestable(tile.ubX, tile.ubY)) {
                mayBeHarvested = 1;
            } else {
                buildingTarget = buildingManagerBuildingAt(tile);
                if (!buildingTarget) {
                    unitTarget = unitManagerUnitAt(tile);
                }
            }
        }
        // trigger actions
        for(UBYTE idx = 0; idx < g_Screen.m_ubSelectedUnitCount; ++idx) {
            Unit *unit = g_Screen.m_pSelectedUnit[idx];
            if (unit) {
                UBYTE worker = unit->type == UNIT_PEON || unit->type == UNIT_PEASANT;
                if (mayBeHarvested && worker) {
                    actionHarvestTile(unit, tile);
                } else if (buildingTarget) {
                    if (worker && buildingTarget->type == BUILDING_GOLD_MINE) {
                        actionHarvestMine(unit, buildingTarget);
                    } else if (buildingTarget->owner != g_ubThisPlayer) {
                        actionAttackBuilding(unit, buildingTarget);
                    } else if (worker && buildingTarget->hp < buildingTypeMaxHealth(&BuildingTypes[buildingTarget->type])) {
                        actionRepair(unit, buildingTarget);
                    } else {
                        actionMoveTo(unit, tile);
                    }
                } else if (unitTarget) {
                    if (unitTarget->owner != g_ubThisPlayer) {
                        actionAttackUnit(unit, unitTarget);
                    } else {
                        actionFollow(unit, unitTarget);
                    }
                } else {
                    actionMoveTo(unit, tile);
                }
            } else {
                break;
            }
        }
    } else if (framesWithoutMouseMove > 15) {
        framesWithoutMouseMove = 0;
        showToolTip();
    }
}

void colorCycle(void) {
    // if ((GameCycle & 31) == 31) {
    //     if (GameCycle & 32) {
    //         // XXX: hardcoded gpl indices
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColors];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColors];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[5]);
    //     } else if (GameCycle & 64) {
    //         // XXX: hardcoded gpl indices
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColors];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColors];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[3]);
    //     } else {
    //         tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.mapColors];
    //         copSetMoveVal(&pCmds[12].sMove, g_Screen.m_map.m_pPalette[12]);
    //         pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.mapColors];
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
        tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.topPanelColors];
        copSetMoveVal(&pCmds[blue1].sMove, red1);
        copSetMoveVal(&pCmds[blue2].sMove, red2);
        copSetMoveVal(&pCmds[blue3].sMove, red3);
        pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.topPanelColors];
        copSetMoveVal(&pCmds[blue1].sMove, red1);
        copSetMoveVal(&pCmds[blue2].sMove, red2);
        copSetMoveVal(&pCmds[blue3].sMove, red3);
    } else if (topHighlight) {
        topHighlight = 0;
        tCopCmd *pCmds = &g_Screen.m_pView->pCopList->pBackBfr->pList[s_copOffsets.topPanelColors];
        copSetMoveVal(&pCmds[blue1].sMove, g_Screen.m_panels.m_pPalette[blue1]);
        copSetMoveVal(&pCmds[blue2].sMove, g_Screen.m_panels.m_pPalette[blue2]);
        copSetMoveVal(&pCmds[blue3].sMove, g_Screen.m_panels.m_pPalette[blue3]);
        pCmds = &g_Screen.m_pView->pCopList->pFrontBfr->pList[s_copOffsets.topPanelColors];
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
    minimapUpdate(); // update 2 lines of minimap

    handleInput();
    colorCycle();
    fowUpdate();
    updateDisplay();
}

void gameGsLoop(void) {
    displayLoop();
    logicLoop();
    if (s_pNewState) {
        if (s_pNewState == g_pGameState) {
            statePop(g_pGameStateManager);
        } else {
            statePush(g_pGameStateManager, s_pNewState);
        }        
    }
}

void gameGsDestroy(void) {
    systemUse();

    screenDestroy();
    unitManagerDestroy();
    buildingManagerDestroy();
}

void logMessage(enum Messages msg) {
    static const char * const str[MSG_COUNT] = {
        "Nothing to harvest there",
        "Nothing more to harvest there",
        "Cannot find a depot",
        "Cannot reach my goal",
        "Cannot build here",
        "Not enough resources",
        "Too many buildings already",
        "Training queue is full",
        "Open main menu",
        //
        [MSG_HOVER_ICON + ICON_NONE] = "",
        [MSG_HOVER_ICON + ICON_FRAME] = "",
        [MSG_HOVER_ICON + ICON_STOP] = "Stop",
        [MSG_HOVER_ICON + ICON_HFARM] = "Human Farm",
        [MSG_HOVER_ICON + ICON_HBARRACKS] = "Human Barracks",
        [MSG_HOVER_ICON + ICON_HLUMBERMILL] = "Human Lumbermill",
        [MSG_HOVER_ICON + ICON_HSMITHY] = "Human Smithy",
        [MSG_HOVER_ICON + ICON_HHALL] = "Human Townhall",
        [MSG_HOVER_ICON + ICON_HARVEST] = "Harvest or Repair",
        [MSG_HOVER_ICON + ICON_CANCEL] = "Cancel",
        [MSG_HOVER_ICON + ICON_SHIELD1] = "Shield",
        [MSG_HOVER_ICON + ICON_PEASANT] = "Peasant",
        [MSG_HOVER_ICON + ICON_MOVE] = "Move",
        [MSG_HOVER_ICON + ICON_ATTACK] = "Attack",
        [MSG_HOVER_ICON + ICON_UNUSED2] = "Unused",
        [MSG_HOVER_ICON + ICON_BUILD_BASIC] = "Build basic structure",
        //
        [MSG_HOVER_UNIT + UNIT_PEASANT] = "A Human Peasant",
        [MSG_HOVER_UNIT + UNIT_PEON] = "",
        [MSG_HOVER_UNIT + UNIT_FOOTMAN] = "",
        [MSG_HOVER_UNIT + UNIT_GRUNT] = "",
        [MSG_HOVER_UNIT + UNIT_ARCHER] = "",
        [MSG_HOVER_UNIT + UNIT_SPEARMAN] = "",
        [MSG_HOVER_UNIT + UNIT_CATAPULT] = "",
        [MSG_HOVER_UNIT + UNIT_KNIGHT] = "",
        [MSG_HOVER_UNIT + UNIT_RAIDER] = "",
        [MSG_HOVER_UNIT + UNIT_CLERIC] = "",
        [MSG_HOVER_UNIT + UNIT_NECROLYTE] = "",
        [MSG_HOVER_UNIT + UNIT_CONJURER] = "",
        [MSG_HOVER_UNIT + UNIT_WARLOCK] = "",
        [MSG_HOVER_UNIT + UNIT_SPIDER] = "",
        [MSG_HOVER_UNIT + UNIT_DAEMON] = "",
        [MSG_HOVER_UNIT + UNIT_ELEMENTAL] = "",
        [MSG_HOVER_UNIT + UNIT_OGRE] = "",
        [MSG_HOVER_UNIT + UNIT_SLIME] = "",
        [MSG_HOVER_UNIT + UNIT_THIEF] = "",
        //
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_TOWNHALL] = "A Human Townhall",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_FARM] = "A Human Farm",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_BARRACKS] = "A Human Barracks",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_LUMBERMILL] = "A Human Lumbermill",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_SMITHY] = "",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_STABLES] = "",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_CHURCH] = "",
        [MSG_HOVER_BUILDING + BUILDING_HUMAN_TOWER] = "",
        [MSG_HOVER_BUILDING + BUILDING_GOLD_MINE] = ""
    };

    blitCopyAligned(g_Screen.m_panels.m_pMainPanelBackground, 96, 6, g_Screen.m_panels.m_pMainPanelBuffer->pBack, 96, 6, 256 - 96, 10);
    if (msg == MSG_COUNT) {
        return;
    }
    tTextBitMap *bmp = g_Screen.m_pMessageBitmaps[msg];
    if (!bmp->pBitMap) {
        const char * const m = str[msg];
        assert(m, "Missing message");
        bmp = g_Screen.m_pMessageBitmaps[msg] = fontCreateTextBitMapFromStr(g_Screen.m_fonts.m_pNormalFont, m);
    }
    fontDrawTextBitMap(g_Screen.m_panels.m_pMainPanelBuffer->pBack, bmp, 96, 6, 1, 0);
}
