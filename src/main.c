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
  logWrite("Hello, Amiga!\n");
  keyCreate(); // We'll use keyboard
  mouseCreate(MOUSE_PORT_1 | MOUSE_PORT_2);
  // Initialize gamestate
  g_pGameStateManager = stateManagerCreate();

  // XXX: these are filled by the menu UI
  g_Map.path = "game2";

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
  logWrite("Goodbye, Amiga!\n");
}
