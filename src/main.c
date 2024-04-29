#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/state.h>
// Without it compiler will yell about undeclared gameGsCreate etc
#include "game.h"
#include "include/map.h"

tStateManager *g_pGameStateManager = 0;
tState *g_pGameState = 0;

void genericCreate(void) {
  keyCreate(); // We'll use keyboard
  mouseCreate(MOUSE_PORT_1 | MOUSE_PORT_2);
  // mouse uses colors 17, 18
  g_pCustom->color[17] = 0x0ccc;
  g_pCustom->color[18] = 0x0888;
  // minimap rectangle is light gray, using color 1 of sprites 0 and 1 (17 & 21)
  g_pCustom->color[21] = 0x0ccc;
  // all the selection rects use the 4th color (glowing green)
  for (int i = 19; i < 32; i += 4) {
    g_pCustom->color[i] = 0x02f4;
  }

  // most everything else is a glowing green
  for (int i = 23; i < 32; ++i) {
    g_pCustom->color[i] = 0x02f4;
  }

  // Initialize gamestate
  g_pGameStateManager = stateManagerCreate();

  // TODO: these are filled by the menu UI
  mapInitialize();
  g_Map.m_pName = "example";

  g_pGameState = stateCreate(gameGsCreate, gameGsLoop, gameGsDestroy, 0, 0, 0);

  statePush(g_pGameStateManager, g_pGameState);
}

void genericProcess(void) {
  keyProcess();
  mouseProcess();
  stateProcess(g_pGameStateManager); // Process current gamestate's loop
}

void genericDestroy(void) {
  stateManagerDestroy(g_pGameStateManager);
  stateDestroy(g_pGameState);
  keyDestroy();
}
