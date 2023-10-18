#include "selection_sprites.h"

#include <ace/generic/screen.h>
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
    SPRITE_DATA,
    SPRITE_DATA,
    SPRITE_DATA
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
}

void selectionSpritesUpdate(UBYTE selectionIdx, WORD selectionX, WORD selectionY) {
    if (selectionX < 0) {
        s_spriteData[selectionIdx][0] = 0;
        s_spriteData[selectionIdx][1] = 0;
        return;
    }
    UWORD hstart = selectionX + 128;
    UWORD vstart = selectionY + SCREEN_PAL_YOFFSET;
    UWORD vstop = vstart + 15;
    s_spriteData[selectionIdx][0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_spriteData[selectionIdx][1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart & 1); /* HSTART low bit 0 */
}
