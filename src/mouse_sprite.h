#ifndef MOUSE_SPRITE_H
#define MOUSE_SPRITE_H

#include <ace/utils/extview.h>

static inline UBYTE mouseSpriteGetRawCopplistInstructionCountLength() {
    return 2;
}

/**
 * @brief Setup the mouse sprite's movement cop commands.
 * 
 * @param pView - the view into which to store the cop commands
 * @param copListStart - at which index into the cop list to insert the commands
 */
void mouseSpriteSetup(tView *pView, UWORD copListStart);

/**
 * @brief Update the mouse's sprite data with the current position.
 * 
 * @param mouseX
 * @param mouseY
 */
void mouseSpriteUpdate(UWORD mouseX, UWORD mouseY);

#endif
