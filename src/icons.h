#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>
#include <ace/utils/bitmap.h>

typedef struct {
    uint8_t *iconSrcPtr;
    uint8_t *iconDstPtr;
    uint16_t dstModulo;
    uint16_t bltsize;
#ifdef ACE_DEBUG
    uint8_t bpp;
#endif
} tIcon;

void iconInit(tIcon *icon,
    uint8_t width, uint8_t height,
    tBitMap *iconTileMap, uint8_t iconIdx,
    tBitMap *iconBuffer, tUwCoordYX iconPosition);

void iconSetSource(tIcon *icon, tBitMap *iconTileMap, uint8_t iconIdx);

void iconDraw(tIcon *icon);

#endif
