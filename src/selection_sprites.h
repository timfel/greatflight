#ifndef SELECTION_SPRITE_H
#define SELECTION_SPRITE_H

#include <stdint.h>
#include <ace/utils/extview.h>

#define NUM_SELECTION 4

static inline uint8_t selectionSpritesGetRawCopplistInstructionCountLength() {
    return 2 * NUM_SELECTION + 4 /* selection rectangle sprite */;
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
 * @param selectionX xPos. if <0, sprite is disabled
 * @param selectionY yPos
 */
void selectionSpritesUpdate(uint8_t selectionIdx, int16_t selectionX, int16_t selectionY);

/**
 * @brief Update the 3 sprites used for corners of selection rectangles.
 * 
 * @param x1 start x of selection
 * @param x2 current x of mouse
 * @param y1 start y of selection
 * @param y2 current y of mouse
 */
void selectionRectangleUpdate(int16_t x1, int16_t x2, int16_t y1, int16_t y2);

#endif
