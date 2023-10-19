#ifndef ICONS_H
#define ICONS_H

#include "include/units.h"
#include <ace/utils/bitmap.h>

typedef enum  __attribute__ ((__packed__)) {
    ICON_NONE = 0,
    ICON_ATTACK,
    ICON_STOP,
    ICON_HFARM,
    ICON_HBARRACKS,
    ICON_HLUMBERMILL,
    ICON_HSMITHY,
    ICON_HHALL,
    ICON_HARVEST,
    ICON_CANCEL,
    ICON_SHIELD1,
    ICON_UNUSED,
    ICON_MOVE,
    ICON_UNUSED1,
    ICON_UNUSED2,
    ICON_BUILD_BASIC,
    //
    ICON_MAX
} IconIdx;
_Static_assert(sizeof(IconIdx) == sizeof(UBYTE), "IconIdx too big");

typedef void (*tIconAction)(Unit **unit, UBYTE unitc);
typedef void (*tIconActionWithTarget)(Unit **unit, UBYTE unitc, tUbCoordYX tilePos);

typedef struct {
    UBYTE *iconSrcPtr;
    UBYTE *iconDstPtr;
    UWORD dstModulo;
    UWORD bltsize;
    tIconAction action;
#ifdef ACE_DEBUG
    UBYTE bpp;
#endif
} tIcon;

typedef struct {
    IconIdx icons[3];
    tIconAction actions[3];
} IconDefinitions;

extern IconDefinitions g_UnitIconDefinitions[];

void iconInit(tIcon *icon,
    UBYTE width, UBYTE height,
    tBitMap *iconTileMap, IconIdx iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition);

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, IconIdx iconIdx);
void iconSetAction(tIcon *icon, tIconAction action);
void iconDraw(tIcon *icon, UBYTE drawAfterOtherIcon);

// Now all the actual action handling
void iconActionMove(Unit **unit, UBYTE unitc);
void iconActionStop(Unit **unit, UBYTE unitc);
void iconActionAttack(Unit **unit, UBYTE unitc);
void iconActionHarvest(Unit **unit, UBYTE unitc);

#endif
