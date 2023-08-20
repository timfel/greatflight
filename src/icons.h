#ifndef ICONS_H
#define ICONS_H

#include <ace/utils/bitmap.h>

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
    tBitMap *iconTileMap, UBYTE iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition);

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, UBYTE iconIdx);

void iconDraw(tIcon *icon);

#endif
