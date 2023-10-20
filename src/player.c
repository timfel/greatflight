#include "include/player.h"

struct Player g_pPlayers[2] = {0};

void playersLoad(tFile *map) {
    UBYTE playerinfo;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[0].gold = playerinfo * 100;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[0].lumber = playerinfo * 100;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[0].aiscript = playerinfo;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[1].gold = playerinfo << 8;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[1].lumber = playerinfo << 8;
    fileRead(map, &playerinfo, 1);
    g_pPlayers[1].aiscript = playerinfo;
}

void savePlayerInfo(tFile *map) {
    UBYTE playerinfo;
    playerinfo = g_pPlayers[0].gold / 100;
    fileWrite(map, &playerinfo, 1);
    playerinfo = g_pPlayers[0].lumber / 100;
    fileWrite(map, &playerinfo, 1);
    playerinfo = g_pPlayers[0].aiscript;
    fileWrite(map, &playerinfo, 1);
    playerinfo = g_pPlayers[1].gold >> 8;
    fileWrite(map, &playerinfo, 1);
    playerinfo = g_pPlayers[1].lumber >> 8;
    fileWrite(map, &playerinfo, 1);
    playerinfo = g_pPlayers[1].aiscript;
    fileWrite(map, &playerinfo, 1);
}
