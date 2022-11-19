#ifndef MOUSE_SPRITE_H
#define MOUSE_SPRITE_H

#include <stdint.h>
#include <ace/utils/extview.h>

static inline uint8_t mouseSpriteGetRawCopplistInstructionCountLength() {
    return 2;
}

/**
 * @brief Setup the mouse sprite's movement cop commands.
 * 
 * @param pView - the view into which to store the cop commands
 * @param copListStart - at which index into the cop list to insert the commands
 */
void mouseSpriteSetup(tView *pView, uint16_t copListStart);

/**
 * @brief Update the mouse's sprite data with the current position.
 * 
 * @param mouseX
 * @param mouseY
 */
void mouseSpriteUpdate(uint16_t mouseX, uint16_t mouseY);

#endif
