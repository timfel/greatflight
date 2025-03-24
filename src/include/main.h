#ifndef MAIN_H
#define MAIN_H

#include <ace/managers/state.h>

#define MENU_ST

enum GameState {
    STATE_MAIN_MENU,
    STATE_INGAME
};

extern tState g_pGameStates[];
extern tStateManager *g_pGameStateManager;

#endif // MAIN_H