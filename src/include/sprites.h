#ifndef SPRITES_H
#define SPRITES_H

#include <ace/utils/extview.h>

UBYTE mouseSpriteGetRawCopplistInstructionCountLength();

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

#define NUM_SELECTION 6

UBYTE selectionSpritesGetRawCopplistInstructionCountLength();

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
 * @brief Update the minimap's sprite data with the current position.
 */
void minimapSpritesUpdate(UWORD minimapX, UWORD minimapY);

/**
 * @brief Update the icon rectangle's sprite data with the current position.
 */
void iconRectSpritesUpdate(UWORD iconX, UWORD iconY);

#endif
