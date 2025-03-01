#include <ace/utils/font.h>
#include <ace/utils/extview.h>
#include <ace/generic/screen.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/utils/palette.h>
#include <ace/managers/system.h>
#include <ace/managers/sprite.h>
#include <ace/managers/state.h>
#include "include/main.h"
#include "include/map.h"

static tView *s_pView;
static tVPort *s_pViewport;
static tSimpleBufferManager *s_pMenuBuffer;
static tSprite *s_pMouseSprite;
static enum GameState s_newState;

void menuGSCreate(void) {
    s_newState = STATE_MAIN_MENU;
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

    spriteManagerCreate(s_pView, 0);
    systemSetDmaBit(DMAB_SPRITE, 1);
    s_pMouseSprite = spriteAdd(0, bitmapCreateFromPath("resources/ui/mouse.bm", 0));
    spriteSetEnabled(s_pMouseSprite, 1);

    viewLoad(s_pView);
    systemUnuse();
}

static void skirmish() {
    mapInitialize();
    // TODO: map selection
    g_Map.m_pName = "example";
    s_newState = STATE_INGAME;
}

void menuGSLoop(void) {
    static UBYTE cycle = 0;

    if (keyUse(KEY_Q)) {
        gameExit();
    } else if (keyUse(KEY_S)) {
        skirmish();
    }

    s_pMouseSprite->wX = mouseGetX(MOUSE_PORT_1);
    s_pMouseSprite->wY = mouseGetY(MOUSE_PORT_1);
    spriteRequestMetadataUpdate(s_pMouseSprite);

    if (mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
        if (mouseInRect(MOUSE_PORT_1, (tUwRect){.uwX = 19, .uwY = 146, .uwWidth = 28, .uwHeight = 10})) {
            // Quit
            gameExit();
        } else if (mouseInRect(MOUSE_PORT_1, (tUwRect){.uwX = 14, .uwY = 46, .uwWidth = 40, .uwHeight = 11})) {
            // Options
        } else if (mouseInRect(MOUSE_PORT_1, (tUwRect){.uwX = 124, .uwY = 85, .uwWidth = 61, .uwHeight = 11})) {
            skirmish();
        } else if (mouseInRect(MOUSE_PORT_1, (tUwRect){.uwX = 128, .uwY = 170, .uwWidth = 74, .uwHeight = 13})) {
            // Multiplayer
        } else if (mouseInRect(MOUSE_PORT_1, (tUwRect){.uwX = 233, .uwY = 66, .uwWidth = 63, .uwHeight = 13})) {
            // Campaign
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

    spriteProcess(s_pMouseSprite);
    spriteProcessChannel(0);
    viewProcessManagers(s_pView);
    copProcessBlocks();
    vPortWaitUntilEnd(s_pViewport);

    if (s_newState != STATE_MAIN_MENU) {
        statePush(g_pGameStateManager, &g_pGameStates[s_newState]);
    }
}

void menuGSDestroy(void) {
    systemSetDmaBit(DMAB_SPRITE, 0);
    spriteSetEnabled(s_pMouseSprite, 0);
    bitmapDestroy(s_pMouseSprite->pBitmap);
    spriteRemove(s_pMouseSprite);
    spriteManagerDestroy();

    viewDestroy(s_pView);
}

void menuGSSuspend(void) {
    menuGSDestroy();
}

void menuGSResume(void) {
    menuGSCreate();
}
