#ifndef SELECTION_SPRITE_H
#define SELECTION_SPRITE_H

#include <stdint.h>
#include <ace/utils/extview.h>

#define NUM_SELECTION 4

static inline uint8_t selectionSpritesGetRawCopplistInstructionCountLength() {
    return 2 * NUM_SELECTION;
}

/**
 * @brief Setup the selection sprite's movement cop commands.
 * 
 * @param pView - the view into which to store the cop commands
 * @param copListStart - at which index into the cop list to insert the commands
 */
void selectionSpritesSetup(tView *pView, uint16_t copListStart);

/**
 * @brief Update the selection's sprite data with the current position.
 * 
 * @param selectionIdx which rectangle to update our of NUM_SELECTION
 * @param selectionX
 * @param selectionY
 */
void selectionSpritesUpdate(uint8_t selectionIdx, uint16_t selectionX, uint16_t selectionY);

#endif