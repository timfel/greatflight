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

#include "screenpart.h"

static tFont *s_pNormalFont;

// initialize method for my sprite manager
static void spriteManagerInit(struct Screenpart *self, UWORD *copper_cmds, ...) {
    struct SpriteManager *this = (struct SpriteManager *)self;
    this->coplistOffset = *copper_cmds;
    *copper_cmds += 16; // 16 copper commands for the sprite manager
}

// destroy the sprite manager
static void spriteManagerDest(struct Screenpart *self) {
    struct SpriteManager *this = (struct SpriteManager *)self;
    systemSetDmaBit(DMAB_SPRITE, 0);
    spriteSetEnabled(this->mouse_sprite, 0);
    bitmapDestroy(this->mouse_sprite->pBitmap);
    spriteManagerDestroy();
}

// build the sprite manager
static void spriteManagerBuild(struct Screenpart *self, tView *view) {
    struct SpriteManager *this = (struct SpriteManager *)self;
    spriteManagerCreate(view, this->coplistOffset, NULL);
    systemSetDmaBit(DMAB_SPRITE, 1);
    this->mouse_sprite = spriteAdd(0, bitmapCreateFromPath("resources/ui/mouse.bm", 0));
    spriteSetEnabled(this->mouse_sprite, 1);
}

// static sprite manager instance
static struct SpriteManager s_spriteManager = {
    .base = {
        .initialize = spriteManagerInit,
        .destroy = spriteManagerDest,
        .build = spriteManagerBuild,
    }
};

// initialize method for the top bar
static void topBarInit(struct Screenpart *self, UWORD *copper_cmds, ...) {
    struct TopBar *this = (struct TopBar *)self;

    // get the UBYTE bpp from the first variadic argument, the UBYTE height from the second
    va_list args;
    va_start(args, copper_cmds);
    this->bpp = (UBYTE)va_arg(args, ULONG);
    this->height = (UBYTE)va_arg(args, ULONG);
    va_end(args);

    this->copListOffset = *copper_cmds;
    *copper_cmds += simpleBufferGetRawCopperlistInstructionCount(this->bpp) + (1 << this->bpp);
}

// build the top bar
static void topBarBuild(struct Screenpart *self, tView *view) {
    struct TopBar *this = (struct TopBar *)self;
    this->top_viewport = vPortCreate(0,
        TAG_VPORT_VIEW, view,
        TAG_VPORT_BPP, this->bpp,
        TAG_VPORT_HEIGHT, this->height,
        TAG_END
    );
    this->top_buffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, this->top_viewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_COPLIST_OFFSET, this->copListOffset,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
        TAG_END
    );
    bitmapLoadFromPath(this->top_buffer->pBack, "resources/ui/top.bm", 0, 0);
    UWORD palette[1 << this->bpp];
    paletteLoadFromPath("resources/palettes/top.plt", palette, 1 << this->bpp);
    
    tCopCmd *copperlist = &this->top_viewport->pView->pCopList->pFrontBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(this->bpp);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }
    copperlist = &this->top_viewport->pView->pCopList->pBackBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(this->bpp);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }

    this->gold_text_bitmap = fontCreateTextBitMapFromStr(s_pNormalFont, "0000000");
    this->lumber_text_bitmap = fontCreateTextBitMapFromStr(s_pNormalFont, "0000000");
}

// top bar destroy function
static void topBarDest(struct Screenpart *self) {
    struct TopBar *this = (struct TopBar *)self;
    vPortDestroy(this->top_viewport);
    fontDestroyTextBitMap(this->gold_text_bitmap);
    fontDestroyTextBitMap(this->lumber_text_bitmap);
}

// static top bar instance
static struct TopBar s_topBar = {
    .base = {
        .initialize = topBarInit,
        .destroy = topBarDest,
        .build = topBarBuild,
    }
};

// initialize method for the bottom bar
static void bottomBarInit(struct Screenpart *self, UWORD *copper_cmds, ...) {
    struct BottomBar *this = (struct BottomBar *)self;
    va_list args;
    va_start(args, copper_cmds);
    this->bpp = (UBYTE)va_arg(args, ULONG);
    this->height = (UBYTE)va_arg(args, ULONG);
    va_end(args);
    this->copListOffset = *copper_cmds;
    *copper_cmds += simpleBufferGetRawCopperlistInstructionCount(this->bpp) + (1 << this->bpp);
}

static void bottomBarBuild(struct Screenpart *self, tView *view) {
    struct BottomBar *this = (struct BottomBar *)self;
    this->bottom_viewport = vPortCreate(0,
        TAG_VPORT_VIEW, view,
        TAG_VPORT_BPP, this->bpp,
        TAG_VPORT_HEIGHT, this->height,
        TAG_END
    );
    this->bottom_buffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, this->bottom_viewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_COPLIST_OFFSET, this->copListOffset,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
        TAG_END
    );
    bitmapLoadFromPath(this->bottom_buffer->pBack, "resources/ui/bottom.bm", 0, 0);
    UWORD palette[1 << this->bpp];
    paletteLoadFromPath("resources/palettes/bottom.plt", palette, 1 << this->bpp);
    
    tCopCmd *copperlist = &this->bottom_viewport->pView->pCopList->pFrontBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(this->bpp);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }
    copperlist = &this->bottom_viewport->pView->pCopList->pBackBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(this->bpp);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }
}

static void bottomBarDestroy(struct Screenpart *self) {
    struct BottomBar *this = (struct BottomBar *)self;
    vPortDestroy(this->bottom_viewport);
}

// static bottom bar instance
static struct BottomBar s_bottomBar = {
    .base = {
        .initialize = bottomBarInit,
        .destroy = bottomBarDestroy,
        .build = bottomBarBuild,
    }
};

// initialize method for the map area
static void mapAreaInit(struct Screenpart *self, UWORD *copper_cmds, ...) {
    struct MapArea *this = (struct MapArea *)self;
    this->copListOffset = *copper_cmds;

    va_list args;
    va_start(args, copper_cmds);
    this->bpp = (UBYTE)va_arg(args, ULONG);
    this->height = (UBYTE)va_arg(args, ULONG);
    this->tile_size = (UBYTE)va_arg(args, ULONG);
    va_end(args);

    UBYTE numColumns = 320 / this->tile_size;
    UBYTE numRows = this->height / this->tile_size;

    this->tilemap = bitmapCreateFromPath("resources/tilesets/woodland.bm", 1);
    #if ACE_DEBUG
    if ((this->tilemap->Flags & BMF_INTERLEAVED) == 0) {
        logWrite("ERR: tilemap must be interleaved for copper driven tilebuffer\n");
        return;
    }
    if (bitmapGetByteWidth(this->tilemap) != this->tile_size / 8) {
        logWrite("ERR: tilemap size must match tilebuffer tile size\n");
        return;
    }
    if (this->tilemap->Depth != this->bpp) {
        logWrite("ERR: tilemap depth must match tilebuffer bpp\n");
        return;
    }
    #endif

    UBYTE alignedTilemap = 0;
    this->tilecount = this->tilemap->Rows * this->tilemap->BytesPerRow / (this->tile_size * this->tile_size * this->bpp / 8);
    // we can realign if the tilemap is less than 64k bytes in size
    if (this->tilemap->BytesPerRow * this->tilemap->Rows < (1 << 16)) {
		PLANEPTR plane0 = (PLANEPTR) memAlloc(this->tilemap->BytesPerRow * this->tilemap->Rows + (1 << 16), MEMF_CHIP);
        if (!plane0) {
			logWrite("ERR: Can't alloc aligned tilemap\n");
			return;
		}
        // now find the offset where 64k alignment starts in plane0 and copy the tilemap->Planes[0] to it
        UWORD offset = (1 << 16) - ((ULONG)plane0 + (1 << 16)) % (1 << 16);
        memcpy(plane0 + offset, this->tilemap->Planes[0], this->tilemap->BytesPerRow * this->tilemap->Rows);
#ifndef ACE_DEBUG
        // cannot free if debugging, because metadata is stored before the allocated memory
        memFree(plane0, offset);
        memFree(plane0 + offset + this->tilemap->BytesPerRow * this->tilemap->Rows, (1 << 16) - offset);
#endif
        bitmapDestroy(this->tilemap);
        this->tilemap_planes = (PLANEPTR)(((ULONG)1 << 31) | (ULONG)(plane0 + offset));
        alignedTilemap = 1;
    } else {
        // tilemap is too big to be aligned
        PLANEPTR plane0 = (PLANEPTR) memAlloc(this->tilemap->BytesPerRow * this->tilemap->Rows, MEMF_CHIP);
        if (!plane0) {
			logWrite("ERR: Can't alloc tilemap\n");
			return;
		}
        memcpy(plane0, this->tilemap->Planes[0], this->tilemap->BytesPerRow * this->tilemap->Rows);
        bitmapDestroy(this->tilemap);
        this->tilemap_planes = plane0;
    }

    *copper_cmds += simpleBufferGetRawCopperlistInstructionCount(4) +
        (1 << this->bpp) + // colors
        7 + // map area blitter setup
            //      wait bltbusy
            //      move USEA|USED|MINTERM_A, bltcon0
            //      move 0, bltcon1
            //      move 0xffff, bltafwm
            //      move 0xffff, bltalwm
            //      move 0, bltamod
            //      move bitmapGetByteWidth(buffer->pBack) - (tilesize / 8), bltdmod
        (alignedTilemap ? 1 : 0) + // source plane high word setup if tilemap is aligned to 64k
            //      move (ULONG)plane0 >> 16, bltapth
        numColumns * 2 + // map area blitter column dptrh and dptrl setup
            //      move CONSTANT, bltdpth
            //      move CONSTANT, bltdptl    
        numColumns * numRows * (3 + (alignedTilemap ? 0 : 1));
            // each tile needs 3 moves and a wait for non-aligned tilemap planes,
            // or 2 moves and a wait for 64k aligned tilemap planes
            // we use the fact that bltdpt is left in the right spot if we draw column-wise
            //      wait bltbusy
            //      (move VARIABLE, bltapth)
            //      move VARIABLE, bltaptl
            //      move uwBltsize = ((tilesize * bpp) << 6) | (tilesize / 16), bltsize
}

static tCopperUlong FAR REGPTR s_pBltapt = (tCopperUlong REGPTR)(
	0xDFF000 + offsetof(tCustom, bltapt)
);
static tCopperUlong FAR REGPTR s_pBltdpt = (tCopperUlong REGPTR)(
	0xDFF000 + offsetof(tCustom, bltdpt)
);

static void mapAreaBuild(struct Screenpart *self, tView *view) {
    // set copper danger bit so it can drive the blitter
    g_pCustom->copcon |= 0x02;
    systemSetDmaBit(DMAB_BLITHOG, 1);

    struct MapArea *this = (struct MapArea *)self;
    this->main_viewport = vPortCreate(0,
        TAG_VPORT_VIEW, view,
        TAG_VPORT_BPP, this->bpp,
        TAG_VPORT_HEIGHT, this->height,
        TAG_END
    );
    this->main_buffer = simpleBufferCreate(0,
        TAG_SIMPLEBUFFER_VPORT, this->main_viewport,
        TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
        TAG_SIMPLEBUFFER_COPLIST_OFFSET, this->copListOffset,
        TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
        TAG_END
    );

    UBYTE waitX = 0xdc;
    UBYTE waitY = this->main_viewport->uwOffsY + view->ubPosY - 1;
    UBYTE numColumns = 320 / this->tile_size;
    UBYTE numRows = this->height / this->tile_size;
    UWORD palette[1 << this->bpp];
    paletteLoadFromPath("resources/palettes/woodland.plt", palette, 1 << this->bpp);

    // I set the high bit in the plane ptr if the bitplane is aligned, get it out again
    UBYTE isAligned = (((ULONG)this->tilemap_planes) & ((ULONG)1 << 31)) != 0;
    this->tilemap_planes = (PLANEPTR)((ULONG)this->tilemap_planes & ~((ULONG)1 << 31));

    const UWORD tileFrameBytes = this->tile_size * this->tile_size * this->bpp / 8;
    PLANEPTR pSrcPlane = this->tilemap_planes + tileFrameBytes * 1;
    UWORD bltsize = ((this->tile_size * this->bpp) << 6) | (this->tile_size / 16);

    // setup front copperlist
    tCopCmd *copperlist = &this->main_viewport->pView->pCopList->pFrontBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(4);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }
    copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
    ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
    copperlist++;
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltcon0, USEA|USED|MINTERM_A);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltcon1, 0);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltafwm, 0xFFFF);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltalwm, 0xFFFF);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltamod, 0);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltdmod, bitmapGetByteWidth(this->main_buffer->pFront) - (this->tile_size / 8));
    if (isAligned) {
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
    }
    PLANEPTR pDstPlane = this->main_buffer->pFront->Planes[0];
    for (UWORD c = 0; c < numColumns; ++c) {
        copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
        ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
        copperlist++;
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltdpt->uwHi, (UWORD)((ULONG)pDstPlane >> 16));
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltdpt->uwLo, (UWORD)((ULONG)pDstPlane));
        if (!isAligned) {
            copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
        }
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwLo, (UWORD)((ULONG)pSrcPlane));
        copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltsize, bltsize);
        for (UWORD r = 1; r < numRows; ++r) {
            copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
            ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
            copperlist++;
            if (!isAligned) {
                copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
            }
            copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwLo, (UWORD)((ULONG)pSrcPlane));
            copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltsize, bltsize);
        }
        pDstPlane += this->tile_size / 8;
    }

    // setup back copperlist
    copperlist = &this->main_viewport->pView->pCopList->pBackBfr->pList[this->copListOffset];
    copperlist += simpleBufferGetRawCopperlistInstructionCount(4);
    for (int i = 0; i < (1 << this->bpp); ++i) {
        copSetMove((tCopMoveCmd *)copperlist, &g_pCustom->color[i], palette[i]);
        copperlist++;
    }
    copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
    ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
    copperlist++;
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltcon0, USEA|USED|MINTERM_A);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltcon1, 0);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltafwm, 0xFFFF);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltalwm, 0xFFFF);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltamod, 0);
    copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltdmod, bitmapGetByteWidth(this->main_buffer->pBack) - (this->tile_size / 8));
    if (isAligned) {
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
    }
    pDstPlane = this->main_buffer->pBack->Planes[0];
    for (UWORD c = 0; c < numColumns; ++c) {
        copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
        ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
        copperlist++;
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltdpt->uwHi, (UWORD)((ULONG)pDstPlane >> 16));
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltdpt->uwLo, (UWORD)((ULONG)pDstPlane));
        if (!isAligned) {
            copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
        }
        copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwLo, (UWORD)((ULONG)pSrcPlane));
        copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltsize, bltsize);
        for (UWORD r = 1; r < numRows; ++r) {
            copSetWait((tCopWaitCmd*)copperlist, waitX, waitY);
            ((tCopWaitCmd*)copperlist)->bfBlitterIgnore = 0;
            copperlist++;
            if (!isAligned) {
                copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwHi, (UWORD)((ULONG)pSrcPlane >> 16));
            }
            copSetMove((tCopMoveCmd*)copperlist++, &s_pBltapt->uwLo, (UWORD)((ULONG)pSrcPlane));
            copSetMove((tCopMoveCmd*)copperlist++, &g_pCustom->bltsize, bltsize);
        }
        pDstPlane += this->tile_size / 8;
    }
}

static void mapAreaDestroy(struct Screenpart *self) {
    struct MapArea *this = (struct MapArea *)self;
    vPortDestroy(this->main_viewport);
    memFree(this->tilemap_planes, this->tilecount * this->tile_size * this->tile_size * this->bpp / 8);
}

// static map area instance
static struct MapArea s_mapArea = {
    .base = {
        .initialize = mapAreaInit,
        .destroy = mapAreaDestroy,
        .build = mapAreaBuild,
    }
};

static tView *s_pView;
static enum GameState s_newState;

static void createView() {
    UWORD copperListSize = 0;
    s_spriteManager.base.initialize(&s_spriteManager.base, &copperListSize);
    s_topBar.base.initialize(&s_topBar.base, &copperListSize, 2, 10);
    s_mapArea.base.initialize(&s_mapArea.base, &copperListSize, 4, 128, 16);
    s_bottomBar.base.initialize(&s_bottomBar.base, &copperListSize, 2, 70);

    s_pView = viewCreate(0,
        TAG_VIEW_WINDOW_HEIGHT, 10 + 128 + 70,
        TAG_COPPER_LIST_MODE, COPPER_MODE_RAW,
        TAG_COPPER_RAW_COUNT, copperListSize,
        TAG_END);
    
    s_spriteManager.base.build(&s_spriteManager.base, s_pView);
    s_topBar.base.build(&s_topBar.base, s_pView);
    s_mapArea.base.build(&s_mapArea.base, s_pView);
    s_bottomBar.base.build(&s_bottomBar.base, s_pView);
}

static void destroyView(void) {
    viewDestroy(s_pView);
}

void ingameGsCreate(void) {
    s_newState = STATE_INGAME;
    systemUse();

    s_pNormalFont = fontCreateFromPath("resources/ui/uni54.fnt");

    createView();

    viewLoad(s_pView);
    systemUnuse();

    g_pCustom->color[16] = 0x0000;
    g_pCustom->color[17] = 0x0444;
    g_pCustom->color[18] = 0x0888;
    g_pCustom->color[19] = 0x0ccc;
}

void ingameGsLoop(void) {
    s_spriteManager.mouse_sprite->wX = mouseGetX(MOUSE_PORT_1);
    s_spriteManager.mouse_sprite->wY = mouseGetY(MOUSE_PORT_1);
    spriteRequestMetadataUpdate(s_spriteManager.mouse_sprite);

    if(keyCheck(KEY_ESCAPE)) {
        s_newState = STATE_MAIN_MENU;
    }

    spriteProcess(s_spriteManager.mouse_sprite);
    spriteProcessChannel(0);
    viewProcessManagers(s_pView);
    copSwapBuffers();
    systemIdleBegin();
    vPortWaitUntilEnd(s_bottomBar.bottom_viewport);
    systemIdleEnd();

    if (s_newState != STATE_INGAME) {
        stateChange(g_pGameStateManager, &g_pGameStates[s_newState]);
    }
}

void ingameGsDestroy(void) {
    if (s_pView == NULL) {
        return;
    }

    s_spriteManager.base.destroy(&s_spriteManager.base);
    s_topBar.base.destroy(&s_topBar.base);
    s_mapArea.base.destroy(&s_mapArea.base);
    s_bottomBar.base.destroy(&s_bottomBar.base);
    viewDestroy(s_pView);
    fontDestroy(s_pNormalFont);

    s_pView = NULL;
}
