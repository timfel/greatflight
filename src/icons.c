#include "stdlib.h"

#include "include/icons.h"
#include "include/buildings.h"
#include "include/sprites.h"
#include "include/map.h"
#include "game.h"

#include <ace/utils/custom.h>
#include <ace/managers/blit.h>

void iconInit(tIcon *icon,
    UBYTE width, UBYTE height,
    UWORD maskLeft, UWORD maskRight,
    tBitMap *iconTileMap, IconIdx iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition)
{
#ifdef ACE_DEBUG
    if (!bitmapIsInterleaved(iconTileMap)) {
        logWrite("ERROR: Icons work only with interleaved bitmaps.");
    }
    if (!bitmapIsInterleaved(iconBuffer)) {
        logWrite("ERROR: Icons work only with interleaved bitmaps.");
    }
    icon->bpp = iconBuffer->Depth;
    if (width % 16) {
        logWrite("ERROR: Icons need to be multiples of 16.");
    }
    if (iconPosition.uwX % 8) {
        logWrite("ERROR: Icons need to be multiples of 8.");
    }
#endif
    UWORD blitWords = width >> 4;
    UWORD dstOffs = iconBuffer->BytesPerRow * iconPosition.uwY + (iconPosition.uwX >> 3);

    icon->maskLeft = maskLeft;
    icon->maskRight = maskRight;
    icon->healthValue = NULL;
    icon->dstModulo = bitmapGetByteWidth(iconBuffer) - (blitWords << 1);
    icon->iconDstPtr = iconBuffer->Planes[0] + dstOffs;
    icon->bltsize = ((height * iconTileMap->Depth) << 6) | (width >> 4);
    iconSetSource(icon, iconTileMap, iconIdx);
}

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, IconIdx iconIdx) {
#ifdef ACE_DEBUG
    if (iconTileMap->Depth != icon->bpp) {
        logWrite("ERROR: Icons bpp need to match src and dst");
    }
#endif
    if (iconIdx == ICON_NONE) {
        icon->iconSrcPtr = iconTileMap->Planes[0];
    } else {
        UBYTE height = icon->bltsize >> 6;
        UWORD srcOffs = bitmapGetByteWidth(iconTileMap) * iconIdx * height;
        icon->iconSrcPtr = iconTileMap->Planes[0] + srcOffs;
    }
    icon->healthValue = NULL;
}

void iconSetHealth(tIcon *icon, UWORD *value, UBYTE shift, UBYTE base) {
#ifdef ACE_DEBUG
    if (!(((ULONG)value) & 1) == 0) {
        logWrite("UNALIGNED POINTER FOR HEALTH BAR!\n");
        exit(1);
    }
#endif
    icon->healthValue = value;
    icon->healthShift = shift;
    icon->healthBase = base;
}

void iconDraw(tIcon *icon, UBYTE drawAfterOtherIcon) {
    blitWait();
    if (!drawAfterOtherIcon) {
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = icon->maskLeft;
        g_pCustom->bltalwm = icon->maskRight;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = icon->dstModulo;
    }
    g_pCustom->bltapt = icon->iconSrcPtr;
    g_pCustom->bltdpt = icon->iconDstPtr;
    g_pCustom->bltsize = icon->bltsize;
}

void iconSetUnitAction(tIcon *icon, tIconActionUnit action) {
    icon->unitAction = action;
}

void iconSetBuildingAction(tIcon *icon, tIconActionBuilding action) {
    icon->buildingAction = action;
}

void iconActionMoveTo(Unit **unit, UBYTE unitc, tUbCoordYX tilePos) {
    while (unitc--) {
        actionMoveTo(*unit++, tilePos);
    }
    iconRectSpritesUpdate(0, 0);
    g_Screen.lmbAction = NULL;
}

void iconActionMove(Unit **, UBYTE unitc) {
    if (!unitc) {
        iconRectSpritesUpdate(0, 0);
    }
    g_Screen.lmbAction = &iconActionMoveTo;
}

void iconActionStop(Unit **unit, UBYTE unitc) {
    while (unitc--) {
        actionStop(*unit++);
        iconRectSpritesUpdate(0, 0);
    }
}

void iconActionAttack(Unit **, UBYTE unitc) {
    if (!unitc) {
        iconRectSpritesUpdate(0, 0);
    }
    logWrite("Attack");
}

void iconActionHarvest(Unit **, UBYTE ) {
    logWrite("Harvest");
}

void cannotBuild(void) {
    logWrite("Cannot build here");
}

static UBYTE *s_previousIcons[NUM_ACTION_ICONS];
static tIconActionUnit s_previousActions[NUM_ACTION_ICONS];

void iconCancel(Unit **, UBYTE ) {
    for (UBYTE i = 0; i < NUM_ACTION_ICONS; ++i) {
        g_Screen.m_pActionIcons[i].iconSrcPtr = s_previousIcons[i];
        g_Screen.m_pActionIcons[i].unitAction = s_previousActions[i];
        iconDraw(&g_Screen.m_pActionIcons[i], i);
    }
    iconRectSpritesUpdate(0, 0);
}

#define BUILD_ACTION(name, buildingType, sz) \
    void iconActionBuild ## name ## At(Unit **unit, UBYTE unitc, tUbCoordYX tilePos) { \
        tUbCoordYX buildPos = {.ubX = tilePos.ubX / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR, .ubY = tilePos.ubY / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR}; \
        if (!buildingCanBeAt(buildingType, buildPos, 0)) { \
            cannotBuild(); \
            return; \
        } \
        iconCancel(unit, unitc); \
        g_Screen.lmbAction = NULL; \
        g_Screen.m_cursorBobs.ubCount = 0; \
        actionBuildAt(*unit, buildPos, buildingType); \
    } \
    void iconBuild ## name(Unit **, UBYTE unitc) { \
        if (!unitc) { \
            iconRectSpritesUpdate(0, 0); \
        } \
        g_Screen.lmbAction = &iconActionBuild ## name ## At; \
        g_Screen.m_cursorBobs.pFirstTile = (PLANEPTR)tileIndexToTileBitmapOffset(BuildingTypes[buildingType].tileIdx); \
        g_Screen.m_cursorBobs.ubCount = sz; \
    }

BUILD_ACTION(HumanFarm, BUILDING_HUMAN_FARM, 1);
BUILD_ACTION(HumanBarracks, BUILDING_HUMAN_BARRACKS, 4);
BUILD_ACTION(HumanLumbermill, BUILDING_HUMAN_LUMBERMILL, 4);
BUILD_ACTION(HumanSmithy, BUILDING_HUMAN_SMITHY, 1);
BUILD_ACTION(HumanHall, BUILDING_HUMAN_TOWNHALL, 4);

void iconActionBuildBasic(Unit **, UBYTE) {
    for (UBYTE i = 0; i < NUM_ACTION_ICONS; ++i) {
        s_previousIcons[i] = g_Screen.m_pActionIcons[i].iconSrcPtr;
        s_previousActions[i] = g_Screen.m_pActionIcons[i].unitAction;
    }
    iconSetSource(&g_Screen.m_pActionIcons[0], g_Screen.m_pIcons, ICON_HFARM);
    iconSetSource(&g_Screen.m_pActionIcons[1], g_Screen.m_pIcons, ICON_HBARRACKS);
    iconSetSource(&g_Screen.m_pActionIcons[2], g_Screen.m_pIcons, ICON_HLUMBERMILL);
    iconSetSource(&g_Screen.m_pActionIcons[3], g_Screen.m_pIcons, ICON_HSMITHY);
    iconSetSource(&g_Screen.m_pActionIcons[4], g_Screen.m_pIcons, ICON_HHALL);
    iconSetSource(&g_Screen.m_pActionIcons[5], g_Screen.m_pIcons, ICON_CANCEL);

    iconSetUnitAction(&g_Screen.m_pActionIcons[0], &iconBuildHumanFarm);
    iconSetUnitAction(&g_Screen.m_pActionIcons[1], &iconBuildHumanBarracks);
    iconSetUnitAction(&g_Screen.m_pActionIcons[2], &iconBuildHumanLumbermill);
    iconSetUnitAction(&g_Screen.m_pActionIcons[3], &iconBuildHumanSmithy);
    iconSetUnitAction(&g_Screen.m_pActionIcons[4], &iconBuildHumanHall);
    iconSetUnitAction(&g_Screen.m_pActionIcons[5], &iconCancel);

    iconDraw(&g_Screen.m_pActionIcons[0], 0);
    iconDraw(&g_Screen.m_pActionIcons[1], 1);
    iconDraw(&g_Screen.m_pActionIcons[2], 1);
    iconDraw(&g_Screen.m_pActionIcons[3], 1);
    iconDraw(&g_Screen.m_pActionIcons[4], 1);
    iconDraw(&g_Screen.m_pActionIcons[5], 1);

    iconRectSpritesUpdate(0, 0);
}

IconDefinitions g_UnitIconDefinitions[unitTypeCount] = {
    [dead] = {},
    [peasant] = {.icons = {ICON_HARVEST, ICON_BUILD_BASIC}, .unitActions = {&iconActionHarvest, &iconActionBuildBasic}},
    [peon] = {.icons = {ICON_HARVEST, ICON_BUILD_BASIC}, .unitActions = {&iconActionHarvest, &iconActionBuildBasic}},
};

void iconActionCancelBuild(Building *building) {
    building->action.action = ActionDie;
    building->action.die.ubTimeout = 10; // TODO: extract constant
    building->hp = 0;
    iconRectSpritesUpdate(0, 0);
    // TODO: resources back?
    if (g_Screen.m_pSelectedBuilding == building) {
        g_Screen.m_pSelectedBuilding = NULL;
        g_Screen.m_ubBottomPanelDirty = 1;
    }
}

void iconBuildPeasant(Building *building) {
    if (building->action.action == ActionTrain) {
        if (!building->action.train.u5UnitType2) {
            building->action.train.u5UnitType2 = peasant;
        } else if (!building->action.train.u5UnitType3) {
            building->action.train.u5UnitType3 = peasant;
        } else {
            logWrite("Training queue full\n");
            return;
        }
    } else {
        building->action.action = ActionTrain;
        building->action.train.u5UnitType1 = peasant;
        building->action.train.uwTimeLeft = 0;
    }
    iconRectSpritesUpdate(0, 0);
    if (g_Screen.m_pSelectedBuilding == building) {
        g_Screen.m_ubBottomPanelDirty = 1;
    }
}

IconDefinitions g_BuildingIconDefinitions[BUILDING_TYPE_COUNT] = {
    [BUILDING_HUMAN_FARM] = {},
    [BUILDING_HUMAN_TOWNHALL] = {.icons = {ICON_PEASANT}, .buildingActions = {&iconBuildPeasant}},
};
