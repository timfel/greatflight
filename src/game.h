#ifndef _GAME_H_
#define _GAME_H_

#include "include/icons.h"

#include <ace/generic/screen.h>
#include <ace/managers/copper.h>
#include <ace/managers/log.h>
#include <ace/managers/mouse.h>
#include <ace/types.h>
#include <ace/managers/viewport/camera.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/custom.h>
#include <ace/utils/extview.h>
#include <ace/utils/file.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>

// Function headers from game.c go here
// It's best to put here only those functions which are needed in other files.

#define BPP 4
#define COLORS (1 << BPP)
#define MAP_WIDTH 320
#define MAP_HEIGHT 160
#define MAP_BUFFER_WIDTH (MAP_WIDTH + TILE_SIZE)
#define MAP_BUFFER_WIDTH_BYTES (MAP_BUFFER_WIDTH / 8)
#define MAP_BUFFER_BYTES_PER_ROW (MAP_BUFFER_WIDTH_BYTES * BPP)
#define MAP_BUFFER_HEIGHT (MAP_HEIGHT + TILE_SIZE)
#define TOP_PANEL_HEIGHT 10
#define BOTTOM_PANEL_HEIGHT 70
#define MINIMAP_OFFSET_X 8 // TODO: adapt graphics
#define MINIMAP_OFFSET_Y 3
#define MINIMAP_WIDTH 64
#define MINIMAP_MODULO (320 / 8 * BPP)

#define NUM_UNIT_ICONS 6
#define NUM_ACTION_ICONS 6

struct Screen {
    tView *m_pView; // View containing all the viewports

    struct {
        UWORD m_pPalette[COLORS];
        // Viewport for resources
        tVPort *m_pTopPanel;
        tSimpleBufferManager *m_pTopPanelBuffer;
        tBitMap *s_pTopPanelBackground;
        
        tTextBitMap *m_pGoldTextBitmap;
        tTextBitMap *m_pLumberTextBitmap;
        tTextBitMap *m_pUnitNameBitmap;

        // Viewport for main panel
        tVPort *m_pMainPanel;
        tSimpleBufferManager *m_pMainPanelBuffer;
        tBitMap *m_pMainPanelBackground;
    } m_panels;

    struct {
        tFont *m_pNormalFont;
    } m_fonts;

    // either 1 or 4 tiles can be drawn under the cursor
    // tiles are expected to be one after the other
    struct {
        PLANEPTR pFirstTile;
        UBYTE ubCount;
    } m_cursorBobs;

    // icons for main panel (actions and selected units)
    tIcon m_pSelectionIcons[NUM_UNIT_ICONS];
    tIcon m_pActionIcons[NUM_ACTION_ICONS];
    tBitMap *m_pIcons;
    tIconActionUnitTarget lmbAction;

    struct {
        // map viewport
        tVPort *m_pVPort;
        tSimpleBufferManager *m_pBuffer;
        tCameraManager *m_pCamera;
        tBitMap *m_pTilemap;
        UWORD m_pPalette[COLORS];
    } m_map;

    unsigned m_ubBottomPanelDirty:1;
};

void screenInit(void);
void screenDestroy(void);

extern struct Screen g_Screen;

extern ULONG tileIndexToTileBitmapOffset(UBYTE index);

void gameGsCreate(void);
void gameGsLoop(void);
void gameGsDestroy(void);

#endif // _GAME_H_
