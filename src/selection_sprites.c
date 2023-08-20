#include "selection_sprites.h"

#include <ace/managers/copper.h>
#include <ace/utils/custom.h>

// 2 words for 15 rows of pixels, plus 2 words position, plus 2 end of data
#define SIZE_OF_SPRITE_DATA (15 + 15 + 2 + 2)
#define SPRITE_DATA                                         \
    {                                                       \
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
        0b1111111111111111, 0b1111111111111111,             \
        0, 0            /* reserved, must init to 0 0 */    \
    }

/* real boring sprite data */
static UWORD CHIP s_spriteData[NUM_SELECTION][SIZE_OF_SPRITE_DATA] = {
    SPRITE_DATA,
    SPRITE_DATA,
    SPRITE_DATA,
    SPRITE_DATA
};

static UWORD CHIP s_selectionRectangleSpriteUpDown[SIZE_OF_SPRITE_DATA * 2 - 2] = {
    0, 0, /* position control for top corner */
    0b1111111111111110, 0b1111111111111110,
    0b1111111111111110, 0b1111111111111110,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    // no control bits, so the DMA channel continues below
    0, 0, /* position control lower corner */
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b0000000110000000, 0b0000000110000000,
    0b1111111111111110, 0b1111111111111110,
    0b1111111111111110, 0b1111111111111110,
    0, 0, /* end of sprite data */
};

static UWORD CHIP s_selectionRectangleSpriteHorizontal[SIZE_OF_SPRITE_DATA] = {
        0, 0, /* position control for 2nd horizontal corner */
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b1111111111111110, 0b1111111111111110,
        0b0000000110000000, 0b0000000110000000,
        0b1111111111111110, 0b1111111111111110,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0b0000000110000000, 0b0000000110000000,
        0, 0, /* end of sprite data */
};

void selectionSpritesSetup(tView *pView, UWORD copListStart) {
    for (int i = 0; i < NUM_SELECTION; i++) {
        ULONG ulSpriteData = (ULONG)(s_spriteData[i]);
        tCopCmd *pCmds = &pView->pCopList->pBackBfr->pList[copListStart];
        copSetMove(&pCmds[0].sMove, &g_pSprFetch[i + 1].uwHi, ulSpriteData << 16);
        copSetMove(&pCmds[1].sMove, &g_pSprFetch[i + 1].uwLo, ulSpriteData & 0xFFFF);
        pCmds = &pView->pCopList->pFrontBfr->pList[copListStart];
        copSetMove(&pCmds[0].sMove, &g_pSprFetch[i + 1].uwHi, ulSpriteData << 16);
        copSetMove(&pCmds[1].sMove, &g_pSprFetch[i + 1].uwLo, ulSpriteData & 0xFFFF);

        copListStart += 2;
    }
    {
        tCopCmd *pCmds = &pView->pCopList->pBackBfr->pList[copListStart];
        copSetMove(&pCmds[0].sMove, &g_pSprFetch[NUM_SELECTION + 1].uwHi, (ULONG)s_selectionRectangleSpriteUpDown << 16);
        copSetMove(&pCmds[1].sMove, &g_pSprFetch[NUM_SELECTION + 1].uwLo, (ULONG)s_selectionRectangleSpriteUpDown & 0xFFFF);
        copSetMove(&pCmds[2].sMove, &g_pSprFetch[NUM_SELECTION + 2].uwHi, (ULONG)s_selectionRectangleSpriteHorizontal << 16);
        copSetMove(&pCmds[3].sMove, &g_pSprFetch[NUM_SELECTION + 2].uwLo, (ULONG)s_selectionRectangleSpriteHorizontal & 0xFFFF);
        pCmds = &pView->pCopList->pFrontBfr->pList[copListStart];
        copSetMove(&pCmds[0].sMove, &g_pSprFetch[NUM_SELECTION + 1].uwHi, (ULONG)s_selectionRectangleSpriteUpDown << 16);
        copSetMove(&pCmds[1].sMove, &g_pSprFetch[NUM_SELECTION + 1].uwLo, (ULONG)s_selectionRectangleSpriteUpDown & 0xFFFF);
        copSetMove(&pCmds[2].sMove, &g_pSprFetch[NUM_SELECTION + 2].uwHi, (ULONG)s_selectionRectangleSpriteHorizontal << 16);
        copSetMove(&pCmds[3].sMove, &g_pSprFetch[NUM_SELECTION + 2].uwLo, (ULONG)s_selectionRectangleSpriteHorizontal & 0xFFFF);
    }
}

void selectionSpritesUpdate(UBYTE selectionIdx, WORD selectionX, WORD selectionY) {
    if (selectionX < 0) {
        s_spriteData[selectionIdx][0] = 0;
        s_spriteData[selectionIdx][1] = 0;
        return;
    }
    UWORD hstart = selectionX + 128;
    UWORD vstart = selectionY + 44;
    UWORD vstop = vstart + 15;
    s_spriteData[selectionIdx][0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_spriteData[selectionIdx][1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart & 1); /* HSTART low bit 0 */
}

void selectionRectangleUpdate(WORD x1, WORD x2, WORD y1, WORD y2) {
    if (x1 < 0) {
        s_selectionRectangleSpriteUpDown[0] = 0;
        s_selectionRectangleSpriteUpDown[1] = 0;
        s_selectionRectangleSpriteHorizontal[0] = 0;
        s_selectionRectangleSpriteHorizontal[1] = 0;
        return;
    }
    UWORD hstart1 = x1 + 128;
    UWORD hstart2 = x2 + 128;
    UWORD vstart1 = y1 + 44;
    UWORD vstart2 = y2 + 44;
    UWORD vstop1 = vstart1 + 15;
    UWORD vstop2 = vstart2 + 15;
    s_selectionRectangleSpriteUpDown[0] = ((vstart1 & 0xff) << 8) | ((hstart1 >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_selectionRectangleSpriteUpDown[1] = ((vstop1 & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart1 >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop1 >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart1 & 1); /* HSTART low bit 0 */
    s_selectionRectangleSpriteUpDown[32] = ((vstart2 & 0xff) << 8) | ((hstart1 >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_selectionRectangleSpriteUpDown[33] = ((vstop2 & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart2 >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop2 >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart1 & 1); /* HSTART low bit 0 */
    s_selectionRectangleSpriteHorizontal[0] = ((vstart1 & 0xff) << 8) | ((hstart2 >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_selectionRectangleSpriteHorizontal[1] = ((vstop1 & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart1 >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop1 >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart2 & 1); /* HSTART low bit 0 */
}
