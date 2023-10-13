#include "icons.h"

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
    /*
        icon->bltsize >> 6 == height * Depth
        BytesPerRow == widthBytes * Depth
        iconYOffset = iconIdx * height
        srcOffs = BytesPerRow * iconYOffset
                = widthBytes * Depth * iconIdx * height
                = widthBytes * iconIdx * height * Depth
                = widthBytes * iconIdx * bltsize>>6
    */
    if (iconIdx == ICON_NONE) {
        icon->iconSrcPtr = 0;
    } else {
        UBYTE height = icon->bltsize >> 6;
        UWORD srcOffs = bitmapGetByteWidth(iconTileMap) * iconIdx * height;
        icon->iconSrcPtr = iconTileMap->Planes[0] + srcOffs;
    }
}

void iconDraw(tIcon *icon) {
    if (!icon->iconSrcPtr) {
        return;
    }
    blitWait();
    g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
    g_pCustom->bltcon1 = 0;
    g_pCustom->bltafwm = 0xffff;
    g_pCustom->bltalwm = 0xffff;
    g_pCustom->bltamod = 0;
    g_pCustom->bltdmod = icon->dstModulo;
    g_pCustom->bltapt = icon->iconSrcPtr;
    g_pCustom->bltdpt = icon->iconDstPtr;
    g_pCustom->bltsize = icon->bltsize;
}
