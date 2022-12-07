/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if 0
#include <coppertilebuffer.h>
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include <ace/utils/tag.h>
#include <proto/exec.h> // Bartman's compiler needs this

static UBYTE shiftFromPowerOfTwo(UWORD uwPot) {
	UBYTE ubPower = 0;
	while(uwPot > 1) {
		uwPot >>= 1;
		++ubPower;
	}
	return ubPower;
}

#ifdef AMIGA

tCopperTileBufferManager *copperTileBufferCreate(void *pTags, ...) {
	va_list vaTags;
	tCopperTileBufferManager *pManager;
	UWORD uwTileX, uwTileY;
	UBYTE ubBitmapFlags;
	UWORD uwCoplistOff;

	logBlockBegin("copperTileBufferCreate(pTags: %p, ...)", pTags);
	va_start(vaTags, pTags);

	// Feed struct with args
	pManager = memAllocFastClear(sizeof(tCopperTileBufferManager));
	pManager->sCommon.process = (tVpManagerFn)copperTileBufferProcess;
	pManager->sCommon.destroy = (tVpManagerFn)copperTileBufferDestroy;
	pManager->sCommon.ubId = VPM_TILEBUFFER;

	tVPort *pVPort = (tVPort*)tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_VPORT, 0);
	if(!pVPort) {
		logWrite("ERR: No parent viewport (TAG_TILEBUFFER_VPORT) specified!\n");
		goto fail;
	}
	pManager->sCommon.pVPort = pVPort;
	logWrite("Parent VPort: %p\n", pVPort);

	UBYTE ubTileShift = tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_TILE_SHIFT, 0);
	if(!ubTileShift) {
		logWrite("ERR: No tile shift (TAG_TILEBUFFER_TILE_SHIFT) specified!\n");
		goto fail;
	}
	pManager->ubTileShift = ubTileShift;
	pManager->ubTileSize = 1 << ubTileShift;

	pManager->pTileSet = (tBitMap*)tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_TILESET, 0);
	if(!pManager->pTileSet) {
		logWrite("ERR: No tileset (TAG_TILEBUFFER_TILESET) specified!\n");
		goto fail;
	}
	uwTileX = tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_BOUND_TILE_X, 0);
	uwTileY = tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_BOUND_TILE_Y, 0);
	if(!uwTileX || !uwTileY) {
		logWrite(
			"ERR: No tile boundaries (TAG_TILEBUFFER_BOUND_TILE_X or _Y) specified!\n"
		);
		goto fail;
	}

	pManager->pTileData = 0;
	ubBitmapFlags = tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_BITMAP_FLAGS, BMF_CLEAR|BMF_INTERLEAVED);
	uwCoplistOff = tagGet(pTags, vaTags, TAG_COPPERTILEBUFFER_COPLIST_OFFSET, -1);
	if (!copperTileBufferReset(pManager, uwTileX, uwTileY, ubBitmapFlags, uwCoplistOff)) {
        goto fail;
    }

	vPortAddManager(pVPort, (tVpManager*)pManager);

	// find camera manager, create if not exists
	// camera created in scroll bfr
	pManager->pCamera = (tCameraManager*)vPortGetManager(pVPort, VPM_CAMERA);

	// Redraw shouldn't take place here because camera is not in proper pos yet,
	// also pTileData is empty

	va_end(vaTags);
	logBlockEnd("copperTileBufferCreate");
	return pManager;
fail:
	// TODO: proper fail
	va_end(vaTags);
	logBlockEnd("copperTileBufferCreate");
	return 0;
}

void copperTileBufferDestroy(tCopperTileBufferManager*pManager) {
	UWORD uwCol;
	logBlockBegin("copperTileBufferDestroy(pManager: %p)", pManager);

	// Free tile data
	for(uwCol = pManager->uTileBounds.uwX; uwCol--;) {
		memFree(pManager->pTileData[uwCol], pManager->uTileBounds.uwY * sizeof(UBYTE));
	}
	memFree(pManager->pTileData, pManager->uTileBounds.uwX * sizeof(UBYTE *));

	// Free manager
	memFree(pManager, sizeof(tCopperTileBufferManager));

	logBlockEnd("copperTileBufferDestroy");
}

UBYTE copperTileBufferReset(
	tCopperTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UBYTE ubBitmapFlags, UWORD uwCoplistOff
) {
	logBlockBegin(
		"copperTileBufferReset(pManager: %p, uwTileX: %hu, uwTileY: %hu, ubBitmapFlags: %hhu)",
		pManager, uwTileX, uwTileY, ubBitmapFlags
	);

	// Free old tile data
	if(pManager->pTileData) {
		for(UWORD uwCol = pManager->uTileBounds.uwX; uwCol--;) {
			memFree(pManager->pTileData[uwCol], pManager->uTileBounds.uwY * sizeof(UBYTE));
		}
		memFree(pManager->pTileData, pManager->uTileBounds.uwX * sizeof(UBYTE *));
		pManager->pTileData = 0;
	}

	// Init new tile data
	pManager->uTileBounds.uwX = uwTileX;
	pManager->uTileBounds.uwY = uwTileY;
	if(uwTileX && uwTileY) {
		pManager->pTileData = memAllocFast(uwTileX * sizeof(UBYTE*));
		for(UWORD uwCol = uwTileX; uwCol--;) {
			pManager->pTileData[uwCol] = memAllocFastClear(uwTileY * sizeof(UBYTE));
		}
	}

	// Reset scrollManager, create if not exists
	UBYTE ubTileShift = pManager->ubTileShift;
    pManager->pSimple = (tSimpleBufferManager*)vPortGetManager(
		pManager->sCommon.pVPort, VPM_DOUBLEBUFFER
	);
	if (!(pManager->pSimple)) {
		pManager->pSimple = simpleBufferCreate(0,
			TAG_SIMPLEBUFFER_VPORT, pManager->sCommon.pVPort,
			TAG_SIMPLEBUFFER_BOUND_WIDTH, pManager->ubTileSize * (pManager->uTileBounds.uwX + 1),
			TAG_SIMPLEBUFFER_BOUND_HEIGHT, pManager->ubTileSize * (pManager->uTileBounds.uwY + 1),
			TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
            TAG_SIMPLEBUFFER_BITMAP_FLAGS, ubBitmapFlags,
            TAG_SIMPLEBUFFER_COPLIST_OFFSET, uwCoplistOff,
		TAG_DONE);
	}

    tCopList *pCopList = pManager->sCommon.pVPort->pView->pCopList;
	if (pCopList->ubMode & COPPER_MODE_BLOCK) {
        logWrite(
			"ERR: copperTileBuffer cannot be used in block copper mode, must use RAW!\n"
		);
        return 1;
    }

    UWORD off = uwCoplistOff + simpleBufferGetRawCopperlistInstructionCount(pManager->sCommon.pVPort->ubBPP);
	/* move bltcon0, USEC|USED|MINTERM_C - blit C to D */
	pCopList->pFrontBfr->pList[off].sMove.bfUnused = 0;
    pCopList->pFrontBfr->pList[off].sMove.bfDestAddr = offsetof(tCustom, bltcon0);
	pCopList->pFrontBfr->pList[off++].sMove.bfValue = USEC|USED|MINTERM_C;
	/* move bltcon1, 0 - nothing to set up */
	pCopList->pFrontBfr->pList[off].sMove.bfUnused = 0;
	pCopList->pFrontBfr->pList[off].sMove.bfDestAddr = offsetof(tCustom, bltcon1);
	pCopList->pFrontBfr->pList[off++].sMove.bfValue = 0;
	/* move bltcmod, src modulo - tiles are one under the other, so no modulo */
	pCopList->pFrontBfr->pList[off].sMove.bfUnused = 0;
	pCopList->pFrontBfr->pList[off].sMove.bfDestAddr = offsetof(tCustom, bltcmod);
	pCopList->pFrontBfr->pList[off++].sMove.bfValue = 0;
	/* move bltcmod, dst modulo - calculate based on byte width of buffer bitmap */
	pCopList->pFrontBfr->pList[off].sMove.bfUnused = 0;
	pCopList->pFrontBfr->pList[off].sMove.bfDestAddr = offsetof(tCustom, bltdmod);
	pCopList->pFrontBfr->pList[off++].sMove.bfValue = bitmapGetByteWidth(pManager->pSimple->pBack) - ((pManager->ubTileSize >> 4) << 1);

	for (UBYTE x = 0; x < pManager->pSimple->uBfrBounds.uwX >> pManager->ubTileShift; x++) {
		for (UBYTE y = 0; y < pManager->pSimple->uBfrBounds.uwY >> pManager->ubTileShift; y++) {
			for (UBYTE plane = 0; plane < (pManager->pSimple->pBack->Flags & BMF_INTERLEAVED) ? 1 : pManager->sCommon.pVPort->ubBPP; plane++) {
				/* move bltcpt, tile bitmap ptr @ tile */
				copSetMove(&pCopList->pFrontBfr->pList[off++].sMove, &g_pBltcpt->uwHi, (ULONG)(pManager->pTileSet) >> 16);
				copSetMove(&pCopList->pFrontBfr->pList[off++].sMove, &g_pBltcpt->uwLo, (ULONG)(pManager->pTileSet) & 0xFFFF);
				/* move bltdpt, destination bitmap ptr @ tile */
				copSetMove(&pCopList->pFrontBfr->pList[off++].sMove, &g_pBltdpt->uwHi, (ULONG)(pManager->pSimple->pBack) >> 16);
				copSetMove(&pCopList->pFrontBfr->pList[off++].sMove, &g_pBltdpt->uwLo, (ULONG)(pManager->pSimple->pBack) & 0xFFFF);
				/* move bltsize, blit size */
				pCopList->pFrontBfr->pList[off].sMove.bfUnused = 0;
				pCopList->pFrontBfr->pList[off].sMove.bfDestAddr = offsetof(tCustom, bltsize);
				pCopList->pFrontBfr->pList[off].sMove.bfValue = (pManager->ubTileSize << 6) | (pManager->ubTileSize >> 4) & 0x1f;
				/* wait blitter finished */
				pCopList->pFrontBfr->pList[off].sWait.bfBlitterIgnore = 0;
				pCopList->pFrontBfr->pList[off].sWait.bfHE = 0;
				pCopList->pFrontBfr->pList[off].sWait.bfIsSkip = 0;
				pCopList->pFrontBfr->pList[off].sWait.bfIsWait = 1;
				pCopList->pFrontBfr->pList[off].sWait.bfVE = 0;
				pCopList->pFrontBfr->pList[off].sWait.bfWaitX = 0;
				pCopList->pFrontBfr->pList[off].sWait.bfWaitY = 0;
			}
		}
	}

	pManager->uwMarginedWidth = pManager->pSimple->uBfrBounds.uwX;
	pManager->uwMarginedHeight = pManager->pSimple->uBfrBounds.uwY;

	logBlockEnd("copperTileBufferReset()");
    return 0;
}

FN_HOTSPOT
void copperTileBufferProcess(tCopperTileBufferManager *pManager) {
	logBlockBegin("copperTileBufferProcess(pManager: %p)", pManager);

	UBYTE ubTileSize = pManager->ubTileSize;
	UBYTE ubTileShift = pManager->ubTileShift;

	WORD wStartX = MAX(0, (pManager->pCamera->uPos.uwX >> ubTileShift) -1);
	WORD wStartY = MAX(0, (pManager->pCamera->uPos.uwY >> ubTileShift) -1);
	// One of bounds may be smaller than viewport + margin size
	UWORD uwEndX = MIN(
		pManager->uTileBounds.uwX,
		wStartX + (pManager->uwMarginedWidth >> ubTileShift)
	);
	UWORD uwEndY = MIN(
		pManager->uTileBounds.uwY,
		wStartY + (pManager->uwMarginedHeight >> ubTileShift)
	);

	UWORD uwTileOffsY = (wStartY << ubTileShift) & (pManager->uwMarginedHeight - 1);
	for (UWORD uwTileY = wStartY; uwTileY < uwEndY; ++uwTileY) {
		UWORD uwTileOffsX = (wStartX << ubTileShift);
		for (UWORD uwTileX = wStartX; uwTileX < uwEndX; ++uwTileX) {
            blitUnsafeCopyAligned(
                pManager->pTileSet,
                0, pManager->pTileData[uwTileX][uwTileY] << pManager->ubTileShift,
                pManager->pScroll->pBack, uwBfrX, uwBfrY,
                pManager->ubTileSize, pManager->ubTileSize
            );

            tileBufferDrawTileQuick(
				pManager, uwTileX, uwTileY, uwTileOffsX, uwTileOffsY
			);
			uwTileOffsX += ubTileSize;
		}
		uwTileOffsY = (uwTileOffsY + ubTileSize) & (pManager->uwMarginedHeight - 1);
	}

	// Refresh bitplane pointers in simpleBuffer's copperlist
    simpleBufferProcess(pManager->pSimple);

	logBlockEnd("copperTileBufferProcess()");
}

void copperTileBufferRedrawAll(tCopperTileBufferManager *pManager) {
	logBlockBegin("copperTileBufferRedrawAll(pManager: %p)", pManager);

	// just process everything twice, for double buffer
    copperTileBufferProcess(pManager);
    copperTileBufferProcess(pManager);

	logBlockEnd("copperTileBufferRedrawAll()");
}

void tileBufferDrawTile(
	const tTileBufferManager *pManager, UWORD uwTileIdxX, UWORD uwTileIdxY
) {
	// Buffer X coord will overflow dimensions but that's fine 'cuz we need to
	// draw on bitplane 1 as if it is part of bitplane 0.
	UWORD uwBfrY = (uwTileIdxY << pManager->ubTileShift) & (pManager->uwMarginedHeight - 1);
	UWORD uwBfrX = (uwTileIdxX << pManager->ubTileShift);

	tileBufferDrawTileQuick(pManager, uwTileIdxX, uwTileIdxY, uwBfrX, uwBfrY);
}

void tileBufferDrawTileQuick(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY,
	UWORD uwBfrX, UWORD uwBfrY
) {
	// This can't use safe blit fn because when scrolling in X direction,
	// we need to draw on bitplane 1 as if it is part of bitplane 0.
	blitUnsafeCopyAligned(
		pManager->pTileSet,
		0, pManager->pTileData[uwTileX][uwTileY] << pManager->ubTileShift,
		pManager->pScroll->pBack, uwBfrX, uwBfrY,
		pManager->ubTileSize, pManager->ubTileSize
	);
	if(pManager->cbTileDraw) {
		pManager->cbTileDraw(
			uwTileX, uwTileY, pManager->pScroll->pBack, uwBfrX, uwBfrY
		);
	}
}

void tileBufferInvalidateRect(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight
) {
	// Tile x/y ranges in given coord
	UWORD uwStartX = uwX >> pManager->ubTileShift;
	UWORD uwEndX = (uwX+uwWidth) >> pManager->ubTileShift;
	UWORD uwStartY = uwY >> pManager->ubTileShift;
	UWORD uwEndY = (uwY+uwHeight) >> pManager->ubTileShift;

	for(uwX = uwStartX; uwX <= uwEndX; ++uwX) {
		for(uwY = uwStartY; uwY <= uwEndY; ++uwY) {
			tileBufferInvalidateTile(pManager, uwX, uwY);
		}
	}
}

void tileBufferInvalidateTile(
	tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	// FIXME it ain't working right yet
	// if(!tileBufferIsTileOnBuffer(pManager, uwTileX, uwTileY)) {
	// 	return;
	// }

	// Previous state will have one more tile to draw, so only check smaller range
	// omit if not yet drawn on redraw margin - let manager draw it when it
	// will be that tile's turn
	// const tRedrawState *pState = &pManager->pRedrawStates[pManager->ubStateIdx];
	// if(
	// 	pState->pMarginX->wTilePos == uwTileX &&
	// 	pState->pMarginX->wTileCurr <= uwTileY &&
	// 	uwTileY <= pState->pMarginX->wTileEnd
	// ) {
	// 	return;
	// }
	// if(
	// 	pState->pMarginY->wTilePos == uwTileY &&
	// 	pState->pMarginY->wTileCurr <= uwTileX &&
	// 	uwTileX <= pState->pMarginY->wTileEnd
	// ) {
	// 	return;
	// }

	// Add to queue
	tileBufferQueueAdd(pManager, uwTileX, uwTileY);
}

UBYTE tileBufferIsTileOnBuffer(
	const tTileBufferManager *pManager, UWORD uwTileX, UWORD uwTileY
) {
	UBYTE ubTileShift = pManager->ubTileShift;
	UWORD uwStartX = MAX(0, (pManager->pCamera->uPos.uwX >> ubTileShift) -1);
	UWORD uwEndX = uwStartX + ((pManager->uwMarginedWidth >> ubTileShift) - 2);
	UWORD uwStartY = MAX(0, (pManager->pCamera->uPos.uwY >> ubTileShift) -1);
	UWORD uwEndY = uwStartY + ((pManager->uwMarginedHeight >> ubTileShift) - 2);

	if(
		uwStartX <= uwTileX && uwTileX <= uwEndX && uwTileX &&
		uwStartY <= uwTileY && uwTileY <= uwEndY && uwTileY
	) {
		return 1;
	}
	return 0;
}

void tileBufferSetTile(
	tTileBufferManager *pManager, UWORD uwX, UWORD uwY, UWORD uwIdx
) {
 	pManager->pTileData[uwX][uwY] = uwIdx;
	tileBufferInvalidateTile(pManager, uwX, uwY);
}

#endif // AMIGA
#endif
