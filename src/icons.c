#include "include/icons.h"
#include "include/sprites.h"
#include "game.h"

#include <ace/utils/custom.h>
#include <ace/managers/blit.h>

void iconInit(tIcon *icon,
    UBYTE width, UBYTE height,
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
}

void iconDraw(tIcon *icon, UBYTE drawAfterOtherIcon) {
    blitWait();
    if (!drawAfterOtherIcon) {
        g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
        g_pCustom->bltcon1 = 0;
        g_pCustom->bltafwm = 0xffff;
        g_pCustom->bltalwm = 0xffff;
        g_pCustom->bltamod = 0;
        g_pCustom->bltdmod = icon->dstModulo;
    }
    g_pCustom->bltapt = icon->iconSrcPtr;
    g_pCustom->bltdpt = icon->iconDstPtr;
    g_pCustom->bltsize = icon->bltsize;
}

void iconSetAction(tIcon *icon, tIconAction action) {
    icon->action = action;
}

void iconActionMoveReally(Unit **unit, UBYTE unitc, tUbCoordYX tilePos) {
    while (unitc--) {
        actionMoveTo(*unit++, tilePos);
    }
    iconRectSpritesUpdate(0, 0);
    g_Screen.lmbAction = NULL;
}

void iconActionMove(Unit **, UBYTE ) {
    g_Screen.lmbAction = &iconActionMoveReally;
}

void iconActionStop(Unit **unit, UBYTE unitc) {
    while (unitc--) {
        actionStop(*unit++);
        iconRectSpritesUpdate(0, 0);
    }
}

void iconActionAttack(Unit **, UBYTE ) {
    logWrite("Attack");
}

void iconActionHarvest(Unit **, UBYTE ) {
    logWrite("Harvest");
}

void iconBuildHumanFarm(Unit **, UBYTE ) {
    logWrite("Build human farm");
}

void iconBuildHumanBarracks(Unit **, UBYTE ) {
    logWrite("Build human barracks");
}

void iconBuildHumanLumbermill(Unit **, UBYTE ) {
    logWrite("Build human lumbermill");
}

void iconBuildHumanSmithy(Unit **, UBYTE ) {
    logWrite("Build human smithy");
}

void iconBuildHumanHall(Unit **, UBYTE ) {
    logWrite("Build human hall");
}

static UBYTE *s_previousIcons[NUM_ACTION_ICONS];
static tIconAction s_previousActions[NUM_ACTION_ICONS];

void iconCancel(Unit **, UBYTE ) {
    for (UBYTE i = 0; i < NUM_ACTION_ICONS; ++i) {
        g_Screen.m_pActionIcons[i].iconSrcPtr = s_previousIcons[i];
        g_Screen.m_pActionIcons[i].action = s_previousActions[i];
        iconDraw(&g_Screen.m_pActionIcons[i], i);
    }
    iconRectSpritesUpdate(0, 0);
}

void iconActionBuildBasic(Unit **, UBYTE) {
    for (UBYTE i = 0; i < NUM_ACTION_ICONS; ++i) {
        s_previousIcons[i] = g_Screen.m_pActionIcons[i].iconSrcPtr;
        s_previousActions[i] = g_Screen.m_pActionIcons[i].action;
    }
    iconSetSource(&g_Screen.m_pActionIcons[0], g_Screen.m_pIcons, ICON_HFARM);
    iconSetSource(&g_Screen.m_pActionIcons[1], g_Screen.m_pIcons, ICON_HBARRACKS);
    iconSetSource(&g_Screen.m_pActionIcons[2], g_Screen.m_pIcons, ICON_HLUMBERMILL);
    iconSetSource(&g_Screen.m_pActionIcons[3], g_Screen.m_pIcons, ICON_HSMITHY);
    iconSetSource(&g_Screen.m_pActionIcons[4], g_Screen.m_pIcons, ICON_HHALL);
    iconSetSource(&g_Screen.m_pActionIcons[5], g_Screen.m_pIcons, ICON_CANCEL);

    iconSetAction(&g_Screen.m_pActionIcons[0], &iconBuildHumanFarm);
    iconSetAction(&g_Screen.m_pActionIcons[1], &iconBuildHumanBarracks);
    iconSetAction(&g_Screen.m_pActionIcons[2], &iconBuildHumanLumbermill);
    iconSetAction(&g_Screen.m_pActionIcons[3], &iconBuildHumanSmithy);
    iconSetAction(&g_Screen.m_pActionIcons[4], &iconBuildHumanHall);
    iconSetAction(&g_Screen.m_pActionIcons[5], &iconCancel);

    iconDraw(&g_Screen.m_pActionIcons[0], 0);
    iconDraw(&g_Screen.m_pActionIcons[1], 1);
    iconDraw(&g_Screen.m_pActionIcons[2], 1);
    iconDraw(&g_Screen.m_pActionIcons[3], 1);
    iconDraw(&g_Screen.m_pActionIcons[4], 1);
    iconDraw(&g_Screen.m_pActionIcons[5], 1);

    iconRectSpritesUpdate(0, 0);
}

#define PAIR0(n) {.icons = {ICON_NONE, ICON_NONE, ICON_NONE}, .actions = {NULL, NULL, NULL}}
#define PAIR1(n, i, a) {.icons = {i, ICON_NONE, ICON_NONE}, .actions = {a, NULL, NULL}}
#define PAIR2(n, i, a, i2, a2) {.icons = {i, i2, ICON_NONE}, .actions = {a, a2, NULL}}
#define PAIR3(n, i, a, i2, a2, i3, a3) {.icons = {i, i2, i3}, .actions = {a, a2, a3}}
IconDefinitions g_UnitIconDefinitions[unitTypeCount] = {
    PAIR0(dead),
    PAIR2(peasant, ICON_HARVEST, &iconActionHarvest, ICON_BUILD_BASIC, &iconActionBuildBasic),
    PAIR2(peon, ICON_HARVEST, &iconActionHarvest, ICON_BUILD_BASIC, &iconActionBuildBasic),
};
