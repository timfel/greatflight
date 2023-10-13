#ifndef _GAME_H_
#define _GAME_H_

#include "include/icons.h"

#include <ace/generic/screen.h>
#include <ace/managers/bob.h>
#include <ace/managers/copper.h>
#include <ace/managers/log.h>
#include <ace/managers/mouse.h>
#include <ace/types.h>
#include <ace/managers/viewport/camera.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/custom.h>
#include <ace/utils/extview.h>
#include <ace/utils/file.h>
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
#define MAP_BUFFER_HEIGHT (MAP_HEIGHT + TILE_SIZE)
#define TOP_PANEL_HEIGHT 10
#define BOTTOM_PANEL_HEIGHT 70
#define MINIMAP_OFFSET_X 8 // TODO: adapt graphics
#define MINIMAP_OFFSET_Y 3
#define MINIMAP_WIDTH 64
#define MINIMAP_MODULO (320 / 8 * BPP)

struct Screen {
    tView *m_pView; // View containing all the viewports

    struct {
        UWORD m_pPalette[COLORS];
        // Viewport for resources
        tVPort *m_pTopPanel;
        tSimpleBufferManager *m_pTopPanelBuffer;
        tBitMap *s_pTopPanelBackground;
        // Viewport for main panel
        tVPort *m_pMainPanel;
        tSimpleBufferManager *m_pMainPanelBuffer;
        tBitMap *m_pMainPanelBackground;
    } m_panels;

    // icons for main panel (actions and selected units)
    tIcon m_pUnitIcons[6];
    tIcon m_pActionIcons[6];
    tBitMap *m_pIcons;

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

extern struct Screen g_Screen;

extern ULONG tileIndexToTileBitmapOffset(UBYTE index);

void gameGsCreate(void);
void gameGsLoop(void);
void gameGsDestroy(void);

#endif // _GAME_H_
