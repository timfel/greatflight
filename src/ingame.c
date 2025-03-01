#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/custom.h>
#include <ace/utils/font.h>

// Viewports
static tVPort *s_pVpTop;
static tVPort *s_pVpMain;
static tVPort *s_pVpBottom;

// Buffers
static tSimpleBufferManager *s_pTopBuffer;
static tSimpleBufferManager *s_pMainBuffer;
static tSimpleBufferManager *s_pBottomBuffer;

// Font
static tFont *s_pFont;

// Mouse cursor from menu.c
extern tBitMap *s_pCursorBitmap;
extern UWORD s_uwCursorX;
extern UWORD s_uwCursorY;

// Add this at the top of the file
tState g_sStateIngame = {
    .cbCreate = ingameGsCreate,
    .cbLoop = ingameGsLoop,
    .cbDestroy = ingameGsDestroy
};

void ingameGsCreate(void) {
    // Create view
    s_pView = viewCreate(0,
        TAG_VIEW_GLOBAL_CLUT, 1,
        TAG_END
    );

    // Top panel - 8 colors
    s_pVpTop = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 3,  // 8 colors
        TAG_VPORT_HEIGHT, 32,  // Adjust height as needed
        TAG_END
    );
    s_pTopBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_pVpTop,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_END
    );

    // Main map view - 16 colors
    s_pVpMain = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 4,  // 16 colors
        TAG_VPORT_HEIGHT, 160,  // Adjust height as needed
        TAG_END
    );
    s_pMainBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_END
    );

    // Bottom panel - 8 colors
    s_pVpBottom = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 3,  // 8 colors
        TAG_VPORT_HEIGHT, 48,  // Adjust height as needed
        TAG_END
    );
    s_pBottomBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_pVpBottom,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_END
    );

    // Load font
    s_pFont = fontCreate("data/fonts/topaz.fnt");

    // Set up copper lists for different color depths
    viewLoad(s_pView);
}

void ingameGsLoop(void) {
    // Update cursor position
    s_uwCursorX = joyGetMouseX();
    s_uwCursorY = joyGetMouseY();

    // Draw cursor at current position
    blitCopy(
        s_pCursorBitmap, 0, 0,
        s_pMainBuffer->pBack, s_uwCursorX, s_uwCursorY,
        16, 16, MINTERM_COOKIE
    );

    // Process input
    if(keyCheck(KEY_ESCAPE)) {
        gameExit();
        return;
    }

    viewProcessManagers(s_pView);
    copProcessBlocks();
    vPortWaitForEnd(s_pVpMain);
}

void ingameGsDestroy(void) {
    // Destroy font
    fontDestroy(s_pFont);

    // Destroy buffers
    simpleBufferDestroy(s_pBottomBuffer);
    simpleBufferDestroy(s_pMainBuffer);
    simpleBufferDestroy(s_pTopBuffer);

    // Destroy view
    viewDestroy(s_pView);
}
