#ifndef ICONS_H
#define ICONS_H

#include "include/units.h"
#include <ace/utils/bitmap.h>

typedef enum  __attribute__ ((__packed__)) {
    ICON_NONE = 0,
    ICON_FRAME,
    ICON_STOP,
    ICON_HFARM,
    ICON_HBARRACKS,
    ICON_HLUMBERMILL,
    ICON_HSMITHY,
    ICON_HHALL,
    ICON_HARVEST,
    ICON_CANCEL,
    ICON_SHIELD1,
    ICON_PEASANT,
    ICON_MOVE,
    ICON_ATTACK,
    ICON_UNUSED2,
    ICON_BUILD_BASIC,
    //
    ICON_MAX
} IconIdx;
_Static_assert(sizeof(IconIdx) == sizeof(UBYTE), "IconIdx too big");

typedef void (*tIconActionUnit)(Unit **unit, UBYTE unitc);
typedef void (*tIconActionUnitTarget)(Unit **unit, UBYTE unitc, tUbCoordYX tilePos);
typedef void (*tIconActionBuilding)(Building *unit);

typedef struct {
    UBYTE *iconSrcPtr;
    UBYTE *iconDstPtr;
    UWORD dstModulo;
    UWORD bltsize;
    UWORD maskLeft;
    UWORD maskRight;
    union {
        tIconActionUnit unitAction;
        tIconActionBuilding buildingAction;
    };
    UWORD *healthValue;
    UBYTE healthShift;
    UBYTE healthBase;
    IconIdx iconIdx;
#ifdef ACE_DEBUG
    UBYTE bpp;
#endif
} tIcon;

typedef struct {
    IconIdx icons[6];
    union {
        tIconActionUnit unitActions[3];
        tIconActionBuilding buildingActions[6];
    };
} IconDefinitions;

extern IconDefinitions g_UnitIconDefinitions[];
extern IconDefinitions g_BuildingIconDefinitions[];

void iconInit(tIcon *icon,
    UBYTE width, UBYTE height,
    UWORD maskLeft, UWORD maskRight,
    tBitMap *iconTileMap, IconIdx iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition);

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, IconIdx iconIdx);
void iconSetUnitAction(tIcon *icon, tIconActionUnit action);
void iconSetBuildingAction(tIcon *icon, tIconActionBuilding action);
void iconDraw(tIcon *icon, UBYTE drawAfterOtherIcon);
void iconSetHealth(tIcon *icon, UWORD *value, UBYTE shift, UBYTE base);

// Now all the actual unitAction handling
void iconActionMove(Unit **unit, UBYTE unitc);
void iconActionStop(Unit **unit, UBYTE unitc);
void iconActionAttack(Unit **unit, UBYTE unitc);
void iconActionHarvest(Unit **unit, UBYTE unitc);

void iconActionCancelBuild(Building *building);

#endif
