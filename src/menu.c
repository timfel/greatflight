#include <ace/utils/font.h>
#include <ace/utils/extview.h>
#include <ace/generic/screen.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/key.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/utils/palette.h>
#include <ace/managers/system.h>

static tView *s_pView;
static tVPort *s_pViewport;
static tSimpleBufferManager *s_pMenuBuffer;

void menuGSCreate(void) {
    systemUse();
    viewLoad(0);
    s_pView = viewCreate(0,
                        TAG_VIEW_WINDOW_HEIGHT, 240,
                        TAG_VIEW_GLOBAL_PALETTE, 1,
                        TAG_DONE);
    s_pViewport = vPortCreate(0,
                              TAG_VPORT_VIEW, s_pView,
                              TAG_VPORT_BPP, 6,
                              TAG_END);
    s_pMenuBuffer = simpleBufferCreate(0,
                                       TAG_SIMPLEBUFFER_VPORT, s_pViewport,
                                       TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
                                       TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
                                       TAG_END);
    paletteLoadFromPath("resources/palettes/menu.plt", s_pViewport->pPalette, 32);
    bitmapLoadFromPath(s_pMenuBuffer->pBack, "resources/ui/menu_background.bm", 0, 0);

    viewLoad(s_pView);
    systemUnuse();
}

void menuGSLoop(void) {
    static UBYTE cycle = 0;

    if (cycle % 2) {
        if (keyCheck(KEY_Q)) {
            gameExit();
        }
    }

    static UWORD original30, original27;
    switch (++cycle) {
        case 50:
            original30 = s_pViewport->pPalette[30];
            s_pViewport->pPalette[30] = s_pViewport->pPalette[31];
            s_pViewport->pPalette[31] = original30;
            viewUpdateGlobalPalette(s_pView);
            break;
        case 100:
            original27 = s_pViewport->pPalette[27];
            s_pViewport->pPalette[27] = original30;
            viewUpdateGlobalPalette(s_pView);
            break;
        case 150:
            s_pViewport->pPalette[31] = s_pViewport->pPalette[30];
            s_pViewport->pPalette[30] = original30;
            viewUpdateGlobalPalette(s_pView);
            break;
        case 200:
            s_pViewport->pPalette[27] = original27;
            viewUpdateGlobalPalette(s_pView);
            cycle = 0;
    }

    viewProcessManagers(s_pView);
    vPortWaitUntilEnd(s_pViewport);
}

void menuGSDestroy(void) {
    simpleBufferDestroy(s_pMenuBuffer);
    vPortDestroy(s_pViewport);
    viewDestroy(s_pView);
}

void menuGSSuspend(void) {
    menuGSDestroy();
}

void menuGSResume(void) {
    menuGSCreate();
}
