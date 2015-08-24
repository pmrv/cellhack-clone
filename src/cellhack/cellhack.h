#ifndef CELLHACK_H
#define CELLHACK_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "dbg.h"

typedef uint8_t (*CellHack_decide_action) (uint8_t *env, uint8_t energy, uint64_t *memory);

typedef struct Cell {
    // what kind of cell this is, either faction (index into GameState->ai) or 0 for "empty".
    uint8_t type;
    // action, which will be evaluated after other actions (move or split)
    uint8_t deferred_action;
    // how much energy points this cell has
    uint8_t energy;
    // tiny amount of persistent memory each cell has
    uint64_t memory;
    // pointers to (the types) immediate neighbours
    uint8_t env[9];
    struct Cell* neighbours[9];
} Cell;

typedef struct {
    int width;
    int height;
    Cell* cells;
    CellHack_decide_action* ai;
    char** names;
    int rounds;
} GameState;

#define Cellhack_rounds(gs) (gs->rounds)

/* Initializes the game state
 * Makes copy of *ai and **names, so those can be freed while the game still
 * uses them */
GameState* CellHack_init (int width, int height, int num, CellHack_decide_action* ai, char** names);

/* Cleans the game's ressources up */
void CellHack_destroy (GameState* gs);

/* Computes the next game state */
void CellHack_tick (GameState* gs);
#endif
