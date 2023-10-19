#include "include/sprites.h"
#include "game.h"

#include <ace/generic/screen.h>
#include <ace/managers/copper.h>
#include <ace/utils/custom.h>

// 2 words for 15 rows of pixels, plus 2 words position
#define SIZE_OF_MOUSE_SPRITE_DATA (15 + 15 + 2)
#define MOUSE_SPRITE_DATA           \
        0, 0,                       \
        0b1000000000000000, 0x0000, \
        0b1100000000000000, 0x0000, \
        0b1110000000000000, 0x0000, \
        0b1111000000000000, 0x0000, \
        0b1111100000000000, 0x0000, \
        0b1111110000000000, 0x0000, \
        0b1111111000000000, 0x0000, \
        0b1111111100000000, 0x0000, \
        0b1111111110000000, 0x0000, \
        0b1111111111000000, 0x0000, \
        0b1111111111100000, 0x0000, \
        0b1111110000000000, 0x0000, \
        0b1110110000000000, 0x0000, \
        0b1100011000000000, 0x0000, \
        0b1000011000000000, 0x0000

// 2 words for 15 rows of pixels, plus 2 words position
#define SIZE_OF_SELECTION_SPRITE_DATA (15 + 15 + 2)
#define SELECTION_SPRITE_DATA                               \
        0, 0,           /* position control           */    \
        0b1111111111111111, 0b1111111111111111,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1000000000000001, 0b1000000000000001,             \
        0b1111111111111111, 0b1111111111111111


// 2 words for 10 rows of pixels, plus 2 words position
#define SIZE_OF_MINIMAP_RECT_SPRITE_DATA (10 + 10 + 2)
#define MINIMAP_RECT_LEFT                                   \
        0, 0,           /* position control           */    \
        0b1111111111111111, 0b1111111111111111,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1111111111111111, 0b1111111111111111

#define MINIMAP_RECT_RIGHT                                  \
        0, 0,                                               \
        0b1111000000000000, 0b1111000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b0001000000000000, 0b0001000000000000,             \
        0b1111000000000000, 0b1111000000000000

#define SIZE_OF_ICON_RECT_SPRITE_DATA (9 + 9 + 2)
#define ICON_RECT_TOPLEFT                                   \
        0, 0,           /* position control           */    \
        0b1111111111111111, 0b1111111111111111,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000
#define ICON_RECT_TOPRIGHT                                  \
        0, 0,           /* position control           */    \
        0b1111111111000000, 0b1111111111000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000
#define ICON_RECT_BOTLEFT                                   \
        0, 0,           /* position control           */    \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1000000000000000, 0b1000000000000000,             \
        0b1111111111111111, 0b1111111111111111
#define ICON_RECT_BOTRIGHT                                  \
        0, 0,           /* position control           */    \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b0000000001000000, 0b0000000001000000,             \
        0b1111111111000000, 0b1111111111000000

static UWORD CHIP s_spriteData[] = {
    MOUSE_SPRITE_DATA,
    0, 0,
    SELECTION_SPRITE_DATA,
    MINIMAP_RECT_LEFT,
    0, 0,
    SELECTION_SPRITE_DATA,
    MINIMAP_RECT_RIGHT,
    0, 0,
    SELECTION_SPRITE_DATA,
    ICON_RECT_TOPLEFT,
    0, 0,
    SELECTION_SPRITE_DATA,
    ICON_RECT_TOPRIGHT,
    0, 0,
    SELECTION_SPRITE_DATA,
    ICON_RECT_BOTLEFT,
    0, 0,
    SELECTION_SPRITE_DATA,
    ICON_RECT_BOTRIGHT,
    0, 0
};

static UWORD s_selectionSpriteOffsets[] = {
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2,
};

static UWORD s_minimapSpriteOffsets[] = {
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
};

static UWORD s_iconRectSpriteOffsets[] = {
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
    SIZE_OF_MOUSE_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_MINIMAP_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA
        + SIZE_OF_ICON_RECT_SPRITE_DATA
        + 2
        + SIZE_OF_SELECTION_SPRITE_DATA,
};

static void moveSpritePointer(tView *pView, UWORD copListStart, UBYTE spriteIdx, UWORD *uwpSpriteData) {
    copSetMove(&pView->pCopList->pBackBfr->pList[copListStart].sMove,
        &g_pSprFetch[spriteIdx].uwHi, ((ULONG)uwpSpriteData) << 16);
    copSetMove(&pView->pCopList->pBackBfr->pList[copListStart + 1].sMove,
        &g_pSprFetch[spriteIdx].uwLo, ((ULONG)uwpSpriteData) & 0xFFFF);
    copSetMove(&pView->pCopList->pFrontBfr->pList[copListStart].sMove,
        &g_pSprFetch[spriteIdx].uwHi, ((ULONG)uwpSpriteData) << 16);
    copSetMove(&pView->pCopList->pFrontBfr->pList[copListStart + 1].sMove,
        &g_pSprFetch[spriteIdx].uwLo, ((ULONG)uwpSpriteData) & 0xFFFF);
}

UBYTE mouseSpriteGetRawCopplistInstructionCountLength() {
    return 2;
}

void mouseSpriteSetup(tView *pView, UWORD copListStart) {
    moveSpritePointer(pView, copListStart, 0, s_spriteData);
}

UBYTE selectionSpritesGetRawCopplistInstructionCountLength() {
    return 2 * NUM_SELECTION;
}

void selectionSpritesSetup(tView *pView, UWORD copListStart) {
    for (UBYTE i = 0; i < NUM_SELECTION; ++i) {
        UWORD *spriteData = s_spriteData + s_selectionSpriteOffsets[i];
        moveSpritePointer(pView, copListStart, i + 1, spriteData);
        copListStart += 2;
    }
}

void mouseSpriteUpdate(UWORD mouseX, UWORD mouseY) {
    UWORD hstart = mouseX + 128;
    UWORD vstart = mouseY + 44;
    UWORD vstop = vstart + 15;
    s_spriteData[0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_spriteData[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                      ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                      ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                      (hstart & 1); /* HSTART low bit 0 */
}

void selectionSpritesUpdate(UBYTE selectionIdx, WORD selectionX, WORD selectionY) {
    UWORD *spriteData = s_spriteData + s_selectionSpriteOffsets[selectionIdx];
    if (selectionX < 0) {
        // put it offscreen, because the minimap sprites are below
        spriteData[0] = (((SCREEN_NTSC_YOFFSET & 0xff) << 8) | ((0 >> 1) & 0xff)); /* VSTART bits 7-0, HSTART bits 8-1 */
        spriteData[1] = ((((SCREEN_NTSC_YOFFSET + 15) & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                         ((SCREEN_NTSC_YOFFSET >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                         (((SCREEN_NTSC_YOFFSET + 15) >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                         (0 & 1)); /* HSTART low bit 0 */
        return;
    }
    UWORD hstart = selectionX + 128;
    UWORD vstart = selectionY + SCREEN_PAL_YOFFSET;
    UWORD vstop = vstart + 15;
    spriteData[0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    spriteData[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart & 1); /* HSTART low bit 0 */
}

void minimapSpritesUpdate(UWORD minimapX, UWORD minimapY) {
    UWORD *spriteDataL = s_spriteData + s_minimapSpriteOffsets[0];
    UWORD *spriteDataR = s_spriteData + s_minimapSpriteOffsets[1];

    UWORD hstart1 = minimapX + 128;
    UWORD hstart2 = minimapX + 128 + 16;
    UWORD vstart = minimapY + SCREEN_PAL_YOFFSET;
    UWORD vstop = vstart + 10;
    spriteDataL[0] = ((vstart & 0xff) << 8) | ((hstart1 >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    spriteDataL[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                     ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                     ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                     (hstart1 & 1); /* HSTART low bit 0 */
    spriteDataR[0] = ((vstart & 0xff) << 8) | ((hstart2 >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    spriteDataR[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                     ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                     ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                     (hstart2 & 1); /* HSTART low bit 0 */
}

void iconRectSpritesUpdate(UWORD iconX, UWORD iconY) {
    if (!iconX) {
        for (UBYTE i = 0; i < 4; ++i) {
            UWORD *spriteData = s_spriteData + s_iconRectSpriteOffsets[i];
            spriteData[0] = spriteData[1] = 0;
        }
        return;
    }
    UBYTE idx = 0;
    for (UBYTE y = 0; y < 2; ++y) {
        for (UBYTE x = 0; x < 2; ++x, ++idx) {
            UWORD *spriteDataL = s_spriteData + s_iconRectSpriteOffsets[idx];
            UWORD hstart = iconX + 128 + x * 16;
            UWORD vstart = iconY + SCREEN_PAL_YOFFSET + y * 9;
            UWORD vstop = vstart + 9;
            spriteDataL[0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
            spriteDataL[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                            ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                            ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                            (hstart & 1); /* HSTART low bit 0 */
        }
    }
}
