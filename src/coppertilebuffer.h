/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ACE_MANAGERS_VIEWPORT_COPPERTILEBUFFER_H_
#define _ACE_MANAGERS_VIEWPORT_COPPERTILEBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AMIGA

/**
 * Tilemap buffer manager
 * Provides speedy tilemap buffer driven by the copper.
 * 
 * It is always double-buffered, but only needs memory
 * to fill (tileWidth + 1)x(tileHeight + 1), so can be more
 * memory efficient than single-buffered but large ordinary
 * tilebuffers.
 * 
 * It always redraws all tiles on processing, so any blits
 * on top are lost. But this also means that BOBs don't need
 * to undraw themselves.
 * 
 * Modifications to the tilemap can become visible 2 frames
 * later at the earliest - the current or next frame updates the
 * copperlist, the next frame the copper will drive the blitter
 * to draw on the backbuffer, and only then can the buffer be
 * shown.
 * 
 * Requires viewport managers:
 * 	- camera
 */

#include <ace/types.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/viewport/simplebuffer.h>

typedef enum tCopperTileBufferCreateTags {
	/**
	 * @brief Pointer to parent vPort. Mandatory.
	 */
	TAG_COPPERTILEBUFFER_VPORT = (TAG_USER | 1),

	/**
	 * @brief Scrollable area bounds, in tiles. Mandatory.
	 */
	TAG_COPPERTILEBUFFER_BOUND_TILE_X = (TAG_USER | 2),
	TAG_COPPERTILEBUFFER_BOUND_TILE_Y = (TAG_USER | 3),

	/**
	 * @brief Size of tile, given in bitshift. Set to 4 for 16px, 5 for 32px, etc. Mandatory.
	 */
	TAG_COPPERTILEBUFFER_TILE_SHIFT = (TAG_USER | 4),

	/**
	 * @brief Buffer bitmap creation flags. Defaults to BMF_CLEAR|BMF_INTERLEAVED.
	 * 
	 * Consider carefully if you want non-interleaved bitmaps for this. It is
	 * much faster with interleaved.
	 */
	TAG_COPPERTILEBUFFER_BITMAP_FLAGS = (TAG_USER | 5),

	/**
	 * @brief If in raw copper mode, offset on copperlist for placing required
	 * copper instructions, specified in copper instruction count since beginning.
	 */
	TAG_COPPERTILEBUFFER_COPLIST_OFFSET = (TAG_USER | 7),

	/**
	 * @brief Pointer to tileset bitmap. Expects it to have all tiles in single
	 * column. Mandatory.
	 */
	TAG_COPPERTILEBUFFER_TILESET = (TAG_USER | 9)
} tCopperTileBufferCreateTags;

/* types */

typedef struct _tTileBufferManager {
	tVpManager sCommon;
	tCameraManager *pCamera;       ///< Quick ref to Camera
	tSimpleBufferManager *pSimple; ///< actual display buffer manager
	// Manager vars
	tUwCoordYX uTileBounds;       ///< Tile count in x,y
	UBYTE ubTileSize;             ///< Tile size in pixels
	UBYTE ubTileShift;            ///< Tile size in shift, e.g. 4 for 16: 1 << 4 == 16
	UWORD uwMarginedWidth;        ///< Width of visible area + margins
	UWORD uwMarginedHeight;       ///< Height of visible area + margins
	UBYTE **pTileData;            ///< 2D array of tile indices
	tBitMap *pTileSet;            ///< Tileset - one tile beneath another
} tCopperTileBufferManager;

/* globals */

/* functions */

/**
 * Tilemap buffer manager create fn. See TAG_TILEBUFFER_* for available options.
 *
 * After calling this function, be sure to do the following:
 * - set initial pos in camera manager,
 * - fill tilemap on .pTileData with tile indices,
 * - call copperTileBufferRedrawAll()
 *
 * @see copperTileBufferRedrawAll()
 * @see copperTileBufferDestroy()
 */
tCopperTileBufferManager *copperTileBufferCreate(void *pTags, ...);

void copperTileBufferDestroy(tCopperTileBufferManager *pManager);

/**
 * @brief Processes given tile buffer manager.
 *
 * This sets up the copper list for the next frame to redraw tiles.
 *
 * @see tileBufferQueueProcess()
 */
void copperTileBufferProcess(tCopperTileBufferManager *pManager);

UBYTE copperTileBufferReset(
	tCopperTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UBYTE ubBitmapFlags, UBYTE isDblBuf, UWORD uwCoplistOff
);

/**
 * Redraws tiles on whole screen.
 */
void copperTileBufferRedrawAll(tCopperTileBufferManager *pManager);

UBYTE copperTileBufferIsTileOnBuffer(
	const tCopperTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
);

/**
 * @brief Changes tile at given position to another tile.
 *
 * @param pManager The tile manager to be used.
 * @param uwX The X coordinate of tile, in tile-space.
 * @param uwY The Y coordinate of tile, in tile-space.
 * @param uwIdx Index of tile to be placed on given position.
 */
void copperTileBufferSetTile(
	tCopperTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwIdx
);

/**
 * @brief Return the number of copper instructions needed for this buffer.
 * 
 * There is some general setup of the blitter when this tilebuffer processes,
 * and then a set of copper instructions for each blit. They are:
 * 		MOVE tile source ptr into BLTCPTL - 2 cycles
 * 		MOVE tile source ptr into BLTCPTH - 2 cycles
 * 		MOVE tile target ptr into BLTDPTL - 2 cycles
 * 		MOVE tile target ptr into BLTDPTH - 2 cycles
 * 		MOVE blt h<<6 | w>>4 into BLTSIZE - 2 cycles
 * 		WAIT until blitter op is finished - 3 cycles
 * 
 * 	If this starts e.g. a blit of 16x16 pixels on 4 interleaved bitplanes,
 *  i.e., one blit 1 word wide and 64 lines high, the blitter needs
 *  	4 * 64 * 1 = 256 cycles
 * 
 * So a single tile blit costs 269 cycles. On a horizontal line, there are
 * 226 cycles in total, but 27 are used up for memory refresh, disk, audio,
 * and sprite DMA. A 4 bitplane display furthermore needs 80 cycles for DMA.
 * This leaves 119 cycles per line, and the CPU will receive at least every
 * fourth cycle (unless BLTHOG is set), so only 80 cycles are available to
 * the blitter. So a complete draw of a 16x16 tile takes a little over 3
 * scanlines. If we assume the viewport needs 20 tiles horizontally (for e.g.
 * a 304px horizontal viewport with tiles on the left and right partially
 * visible), and 13 tiles vertically (a 192px vertical height), we are going
 * to need 260*4=1040 scanlines to draw the entire viewport. PAL has 625,
 * NTSC 525 lines, so we can draw the entire screen in just under 2 frames.
 * During this time, the CPU runs at roughly half speed because of all the
 * blitter activity. So realistically, we need an additional frame for all
 * the other game logic and additional drawing we may do, yielding an FPS
 * of 20 for NTSC and 16.6 for PAL machines. If we want to speed this up we
 * can set DMAF_BLITHOG and starve the 680x0 out of CHIP DMA cycles (maybe
 * we have FAST RAM available), to gain and additional 39 cycles per line
 * for the blitter. This then allows us to draw the same viewport in roughly
 * 650 scanlines, so just over 1 frame per display, so we could run at
 * 30/25fps.
 * 
 * @param ubBpp desired bitplane count
 * @param uwWidth desired width of tile buffer in tiles
 * @param uwHeight desired height of tile buffer in tiles
 * @return UBYTE 
 */
static inline UBYTE copperTileBufferGetRawCopperlistInstructionCount(UBYTE ubBpp, UBYTE uwWidth, UBYTE uwHeight, UBYTE nonInterleaved) {
    return (
		1 + /* move bltcon0, USEC|USED|MINTERM_C */
		1 + /* move bltcon1, 0 */
		1 + /* move bltcmod, src modulo */
		1   /* move bltdmod, dst modulo */
	) + uwWidth * uwHeight * (nonInterleaved ? ubBpp : 1) * (
		2 + /* move bltcpt, tile bitmap ptr @ tile */
		2 + /* move bltdpt, destination bitmap ptr @ tile */
		1 + /* move bltsize, blit size */
		1   /* wait blitter finished */
	) + simpleBufferGetRawCopperlistInstructionCount(ubBpp);
}

#endif // AMIGA

#ifdef __cplusplus
}
#endif

#endif // _ACE_MANAGERS_VIEWPORT_TILEBUFFER_H_
