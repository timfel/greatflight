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

static tView *s_pView;
static struct TopBar s_topBar;

static tVPort *s_pMainViewport, *s_pBottomViewport;
static tSimpleBufferManager *s_pMainBuffer, *s_pBottomBuffer;
static tSprite *s_pMouseSprite;
static tFont *s_pNormalFont;
static tTextBitMap *s_pGoldTextBitmap, *s_pLumberTextBitmap, *s_pUnitNameBitmap;
static enum GameState s_newState;

static void createTopBar(void) {
    s_topBar.m_pTopViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 2,
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
        4,  // move 4 colors
        1, 1
    );
    UWORD palette[4];
    paletteLoadFromPath("resources/palettes/top.plt", palette, 4);
    copMove(s_topBar.m_pTopViewport->pView->pCopList, s_topBar.m_pCopBlock, &g_pCustom->color[0], palette[0]);
    copMove(s_topBar.m_pTopViewport->pView->pCopList, s_topBar.m_pCopBlock, &g_pCustom->color[1], palette[1]);
    copMove(s_topBar.m_pTopViewport->pView->pCopList, s_topBar.m_pCopBlock, &g_pCustom->color[2], palette[2]);
    copMove(s_topBar.m_pTopViewport->pView->pCopList, s_topBar.m_pCopBlock, &g_pCustom->color[3], palette[3]);
}

static void createBottomBar(void) {
    s_pBottomViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 2,
        TAG_VPORT_HEIGHT, 70,
        TAG_END
    );
    s_pBottomBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_pBottomViewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
        TAG_END
    );
    bitmapLoadFromPath(s_pBottomBuffer->pBack, "resources/ui/bottom.bm", 0, 0);
    tCopBlock *pCopBlock = copBlockCreate(s_pBottomViewport->pView->pCopList,
        4,  // move 4 colors
        1, s_pMainViewport->uwOffsY + s_pMainViewport->uwHeight + s_pView->ubPosY
    );
    UWORD palette[4];
    paletteLoadFromPath("resources/palettes/bottom.plt", palette, 4);
    copMove(s_pBottomViewport->pView->pCopList, pCopBlock, &g_pCustom->color[0], palette[0]);
    copMove(s_pBottomViewport->pView->pCopList, pCopBlock, &g_pCustom->color[1], palette[1]);
    copMove(s_pBottomViewport->pView->pCopList, pCopBlock, &g_pCustom->color[2], palette[2]);
    copMove(s_pBottomViewport->pView->pCopList, pCopBlock, &g_pCustom->color[3], palette[3]);
}

static void createView(void) {
    viewLoad(0);
    s_pView = viewCreate(0,
        TAG_VIEW_WINDOW_HEIGHT, 220,
        TAG_COPPER_LIST_MODE, COPPER_MODE_BLOCK,
        TAG_END);
    createTopBar();
    s_pMainViewport = vPortCreate(0,
        TAG_VPORT_VIEW, s_pView,
        TAG_VPORT_BPP, 4,
        TAG_VPORT_HEIGHT, 140,
        TAG_END
    );
    s_pMainBuffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, s_pMainViewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
        TAG_END
    );
    createBottomBar();
}

static void destroyView(void) {
    viewDestroy(s_pView);
}

void ingameGsCreate(void) {
    s_newState = STATE_INGAME;
    systemUse();

    createView();

    spriteManagerCreate(s_pView, 0, NULL);
    systemSetDmaBit(DMAB_SPRITE, 1);
    s_pMouseSprite = spriteAdd(0, bitmapCreateFromPath("resources/ui/mouse.bm", 0));
    spriteSetEnabled(s_pMouseSprite, 1);

    s_pNormalFont = fontCreateFromPath("resources/ui/uni54.fnt");
    const char *text = "0000000";
    s_pGoldTextBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, text);
    s_pLumberTextBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, text);
    s_pUnitNameBitmap = fontCreateTextBitMapFromStr(s_pNormalFont, text);

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
    vPortWaitUntilEnd(s_pBottomViewport);
    systemIdleEnd();

    if (s_newState != STATE_INGAME) {
        if (s_newState == STATE_PREV) {
            statePop(g_pGameStateManager);
        } else {
            statePush(g_pGameStateManager, &g_pGameStates[s_newState]);
        }
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
