#include "mouse_sprite.h"

#include <ace/managers/copper.h>
#include <ace/utils/custom.h>

/* real boring sprite data */
static uint16_t CHIP s_spriteData[] = {
    0, 0,           /* position control           */
    0b1000000000000000, 0x0000,
    0b1100000000000000, 0x0000,
    0b1110000000000000, 0x0000,
    0b1111000000000000, 0x0000,
    0b1111100000000000, 0x0000,
    0b1111110000000000, 0x0000,
    0b1111111000000000, 0x0000,
    0b1111111100000000, 0x0000,
    0b1111111110000000, 0x0000,
    0b1111111111000000, 0x0000,
    0b1111111111100000, 0x0000,
    0b1111111111110000, 0x0000,
    0b0000110000000000, 0x0000,
    0b0000011000000000, 0x0000,
    0b0000011000000000, 0x0000,
    0, 0            /* reserved, must init to 0 0 */
};

void mouseSpriteSetup(tView *pView, uint16_t copListStart) {
    uint32_t ulSpriteData = (uint32_t)s_spriteData;
    tCopCmd *pCmds = &pView->pCopList->pBackBfr->pList[copListStart];
    copSetMove(&pCmds[0].sMove, &g_pSprFetch[0].uwHi, ulSpriteData << 16);
    copSetMove(&pCmds[1].sMove, &g_pSprFetch[0].uwLo, ulSpriteData & 0xFFFF);
    pCmds = &pView->pCopList->pFrontBfr->pList[copListStart];
    copSetMove(&pCmds[0].sMove, &g_pSprFetch[0].uwHi, ulSpriteData << 16);
    copSetMove(&pCmds[1].sMove, &g_pSprFetch[0].uwLo, ulSpriteData & 0xFFFF);
}

void mouseSpriteUpdate(uint16_t mouseX, uint16_t mouseY) {
    uint16_t hstart = mouseX + 128;
    uint16_t vstart = mouseY + 44;
    uint16_t vstop = vstart + 15;
    s_spriteData[0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff); /* VSTART bits 7-0, HSTART bits 8-1 */
    s_spriteData[1] = ((vstop & 0xff) << 8) | /* VSTOP = height + VSTART bits 7-0 */
                    ((vstart >> 8) & 1) << 2 | /* VSTART hight bit 8 */
                    ((vstop >> 8) & 1) << 1 | /* VSTOP high bit 8 */
                    (hstart & 1); /* HSTART low bit 0 */
}
