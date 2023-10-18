#ifndef SELECTION_SPRITE_H
#define SELECTION_SPRITE_H

#include <ace/utils/extview.h>

#define NUM_SELECTION 6

static inline UBYTE selectionSpritesGetRawCopplistInstructionCountLength() {
    return 2 * NUM_SELECTION + 4 /* selection rectangle sprite */;
}

/**
 * @brief Setup the selection sprite's movement cop commands.
 * 
 * @param pView - the view into which to store the cop commands
 * @param copListStart - at which index into the cop list to insert the commands
 */
void selectionSpritesSetup(tView *pView, UWORD copListStart);

/**
 * @brief Update the selection's sprite data with the current position.
 * 
 * @param selectionIdx which rectangle to update our of NUM_SELECTION
 * @param selectionX xPos. if <0, sprite is disabled
 * @param selectionY yPos
 */
void selectionSpritesUpdate(UBYTE selectionIdx, WORD selectionX, WORD selectionY);

/**
 * @brief Update the selection's sprite data with the current position.
 * 
 * @param selectionIdx which rectangle to update our of NUM_SELECTION
 * @param selectionX xPos. if <0, sprite is disabled
 * @param selectionY yPos
 */
void selectionSpritesUpdate(UBYTE selectionIdx, WORD selectionX, WORD selectionY);

#endif
