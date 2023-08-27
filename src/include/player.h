#ifndef PLAYER_H
#define PLAYER_H

#include "ace/types.h"
#include <ace/utils/file.h>

struct Player {
    UWORD gold;
    UWORD lumber;
    UBYTE aiscript;
};

extern struct Player g_pPlayers[2];

void loadPlayerInfo(tFile *map);
void savePlayerInfo(tFile *map);

#endif