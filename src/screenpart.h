#ifndef SCREENPART_H
#define SCREENPART_H

struct Screenpart {
    // function pointer to initialize method. takes an out-variable for the required copper commands. takes variable arguments
    void (*initialize)(struct Screenpart *self, UWORD *copper_cmds, ...);
    // function pointer to destruction method. no arguments and no return value
    void (*destroy)(struct Screenpart *self);
    // function pointer to the creation method. takes a tView pointer and has no return value
    void (*build)(struct Screenpart *self, tView *view);
};

// structural subclass of Screenpart, the sprite manager
struct SpriteManager {
    struct Screenpart base; // inherit from Screenpart
    union {
        tSprite *mouse_sprite; // bitmap for the mouse sprite
        UWORD coplistOffset;
    };
};

// structural subclass of Screenpart, the top bar
struct TopBar {
    struct Screenpart base; // inherit from Screenpart
    UBYTE bpp;
    UBYTE height;
    tVPort *top_viewport; // viewport for the top bar
    tSimpleBufferManager *top_buffer; // buffer manager for the top bar
    UWORD copListOffset; // offset in the copper list for the top bar
    tTextBitMap *gold_text_bitmap; // bitmap for the gold text
    tTextBitMap *lumber_text_bitmap; // bitmap for the lumber text
};

// structural subclass of Screenpart, the bottom bar
struct BottomBar {
    struct Screenpart base; // inherit from Screenpart
    tVPort *bottom_viewport; // viewport for the bottom bar
    tSimpleBufferManager *bottom_buffer; // buffer manager for the bottom bar
    UWORD copListOffset; // copper list for the bottom bar
    UBYTE bpp;
    UBYTE height;
};

// structural subclass of Screenpart, the map area
struct MapArea {
    struct Screenpart base; // inherit from Screenpart
    tVPort *main_viewport; // viewport for the map area
    tSimpleBufferManager *main_buffer; // buffer manager for the map area
    union {
        tBitMap *tilemap; // bitmap for the tilemap
        PLANEPTR tilemap_planes;
    };
#ifdef ACE_DEBUG
    UWORD planeAlignmentOffset;
#endif
    UBYTE tilecount;
    UBYTE tile_size; // size of the tiles in the tilemap
    UBYTE height;
    UBYTE bpp;
    UWORD copListOffset;
    // pointer to a copper command that sets the bltapt for a tile.
    tCopCmd *tileMoveCommand;
    // function pointer to update the current tileMoveCommand and move tileMoveCommand to the next tile.
    // resets tileMoveCommand to the first tileMoveCommand in the copper list at the end (wrap around)
    void (*next_tile)(struct MapArea *self, UBYTE tile_index);
};

#endif // SCREENPART_H
