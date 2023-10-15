#ifndef ICONS_H
#define ICONS_H

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

typedef struct {
    UBYTE *iconSrcPtr;
    UBYTE *iconDstPtr;
    UWORD dstModulo;
    UWORD bltsize;
#ifdef ACE_DEBUG
    UBYTE bpp;
#endif
} tIcon;

void iconInit(tIcon *icon,
    UBYTE width, UBYTE height,
    tBitMap *iconTileMap, IconIdx iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition);

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, IconIdx iconIdx);

void iconDraw(tIcon *icon);

#endif