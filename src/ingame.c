#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/sprite.h>
#include <ace/managers/mouse.h>
#include <ace/utils/custom.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>

#include "include/main.h"

struct TopBar {
    tSimpleBufferManager *m_pTopBuffer;
    tVPort *m_pTopViewport;
    tTextBitMap *m_pGoldTextBitmap;
    tTextBitMap *m_pLumberTextBitmap;
    tCopBlock *m_pCopBlock;
};

struct BottomBar {
    tSimpleBufferManager *m_pBottomBuffer;
    tVPort *m_pBottomViewport;
    tTextBitMap *m_pUnitNameBitmap;
    tCopBlock *m_pCopBlock;
};

struct MapArea {
    tSimpleBufferManager *m_pMainBuffer;
    tVPort *m_pMainViewport;
    tCopBlock *m_pCopBlock;
};

static tView *s_pView;
static struct TopBar s_topBar;
static struct BottomBar s_bottomBar;
static struct MapArea s_mapArea;

static tSprite *s_pMouseSprite;
static tFont *s_pNormalFont;
static enum GameState s_newState;

static void createTopBar(void) {
    const int bpp = 2;
    s_topBar.m_pTopViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, bpp,
        TAG_VPORT_HEIGHT, 10,
        TAG_END
    );
    s_topBar.m_pTopBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_topBar.m_pTopViewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
        TAG_END
    );
    bitmapLoadFromPath(s_topBar.m_pTopBuffer->pBack, "resources/ui/top.bm", 0, 0);
    s_topBar.m_pCopBlock = copBlockCreate(s_topBar.m_pTopViewport->pView->pCopList,
        1 << bpp,  // move colors
        1, 1
    );
    UWORD palette[1 << bpp];
    paletteLoadFromPath("resources/palettes/top.plt", palette, 1 << bpp);
    for (int i = 0; i < (1 << bpp); ++i) {
        copMove(s_topBar.m_pTopViewport->pView->pCopList, s_topBar.m_pCopBlock, &g_pCustom->color[i], palette[i]);
    }
    s_topBar.m_pGoldTextBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, "0000000");
    s_topBar.m_pLumberTextBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, "0000000");
}

static void createBottomBar(void) {
    const int bpp = 2;
    s_bottomBar.m_pBottomViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, bpp,
        TAG_VPORT_HEIGHT, 70,
        TAG_END
    );
    s_bottomBar.m_pBottomBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_bottomBar.m_pBottomViewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
        TAG_END
    );
    bitmapLoadFromPath(s_bottomBar.m_pBottomBuffer->pBack, "resources/ui/bottom.bm", 0, 0);
    s_bottomBar.m_pCopBlock = copBlockCreate(s_bottomBar.m_pBottomViewport->pView->pCopList,
        1 << bpp,  // move colors
        0xdc, s_mapArea.m_pMainViewport->uwOffsY + s_mapArea.m_pMainViewport->uwHeight + s_pView->ubPosY - 1
    );
    UWORD palette[1 << bpp];
    paletteLoadFromPath("resources/palettes/bottom.plt", palette, 1 << bpp);
    for (int i = 0; i < (1 << bpp); ++i) {
        copMove(s_bottomBar.m_pBottomViewport->pView->pCopList, s_bottomBar.m_pCopBlock, &g_pCustom->color[i], palette[i]);
    }
    s_bottomBar.m_pUnitNameBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, "Unit name");
}

static void createMapArea(void) {
    const int bpp = 4;
    s_mapArea.m_pMainViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, bpp,
        TAG_VPORT_HEIGHT, 140,
        TAG_END
    );
    s_mapArea.m_pMainBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_mapArea.m_pMainViewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
        TAG_END
    );
    s_mapArea.m_pCopBlock = copBlockCreate(s_mapArea.m_pMainViewport->pView->pCopList,
        1 << bpp, // move colors
        0xdc, s_mapArea.m_pMainViewport->uwOffsY + s_pView->ubPosY - 1
    );
    UWORD palette[1 << bpp];
    paletteLoadFromPath("resources/palettes/woodland.plt", palette, 1 << bpp);
    for (int i = 0; i < (1 << bpp); ++i) {
        copMove(s_mapArea.m_pMainViewport->pView->pCopList, s_mapArea.m_pCopBlock, &g_pCustom->color[i], palette[i]);
    }
}

static void createView(void) {
    viewLoad(0);
    s_pView = viewCreate(0,
        TAG_VIEW_WINDOW_HEIGHT, 220,
        TAG_COPPER_LIST_MODE, COPPER_MODE_BLOCK,
        TAG_END);
    createTopBar();
    createMapArea();
    createBottomBar();
}

static void destroyView(void) {
    viewDestroy(s_pView);
}

void ingameGsCreate(void) {
    s_newState = STATE_INGAME;
    systemUse();

    s_pNormalFont = fontCreateFromPath("resources/ui/uni54.fnt");

    createView();

    spriteManagerCreate(s_pView, 0, NULL);
    systemSetDmaBit(DMAB_SPRITE, 1);
    s_pMouseSprite = spriteAdd(0, bitmapCreateFromPath("resources/ui/mouse.bm", 0));
    spriteSetEnabled(s_pMouseSprite, 1);


    viewLoad(s_pView);
    systemUnuse();
}

void ingameGsLoop(void) {
    s_pMouseSprite->wX = mouseGetX(MOUSE_PORT_1);
    s_pMouseSprite->wY = mouseGetY(MOUSE_PORT_1);
    spriteRequestMetadataUpdate(s_pMouseSprite);

    if(keyCheck(KEY_ESCAPE)) {
        s_newState = STATE_MAIN_MENU;
    }

    spriteProcess(s_pMouseSprite);
    spriteProcessChannel(0);
    viewProcessManagers(s_pView);
    copProcessBlocks();
    systemIdleBegin();
    vPortWaitUntilEnd(s_bottomBar.m_pBottomViewport);
    systemIdleEnd();

    if (s_newState != STATE_INGAME) {
        stateChange(g_pGameStateManager, &g_pGameStates[s_newState]);
    }
}

void ingameGsDestroy(void) {
    if (s_pView == NULL) {
        return;
    }
    systemSetDmaBit(DMAB_SPRITE, 0);
    bitmapDestroy(s_pMouseSprite->pBitmap);
    spriteManagerDestroy();

    // Destroy font
    fontDestroy(s_pNormalFont);
    
    destroyView();
    s_pView = NULL;
}
