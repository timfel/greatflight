#ifndef PLAYER_H
#define PLAYER_H

#include "ace/types.h"
#include <ace/utils/file.h>

typedef struct _player {
    UWORD gold;
    UWORD lumber;
    UBYTE aiscript;
} Player;

extern Player g_pPlayers[2];
extern UBYTE g_ubThisPlayer;

void playersLoad(tFile *map);
void savePlayerInfo(tFile *map);

#endif