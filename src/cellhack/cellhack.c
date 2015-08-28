// Copyright 2015 Marvin Poul
// Licensed under the Do What The Fuck You Want To License, Version 2
// See LICENSE for details or http://www.wtfpl.net/txt/copying

#include "cellhack.h"

/* returns an integer from 0 to max exclusive */
unsigned int
randint (unsigned int max)
{
    return rand () % max; // can't be arsed about modulo bias just yet
}

GameState*
CellHack_init (int width, int height, int num,
               CellHack_decide_action* ai, char** names)
{
    GameState *gs = NULL;
    check (num > 0, "Must load at least one cell faction.");

    // all starting cells are arranged in a square
    // number of starting cells per side of the square is
    int side_length = (int) ceilf (sqrtf ((float) num));
    check (side_length < width && side_length < height,
           "Too many players to fit on the field.");

    gs = calloc (1, sizeof (GameState));
    check (gs != NULL, "Failed to alloc memory for game state.");

    gs->ai = calloc (num, sizeof (CellHack_decide_action));
    check (gs->ai != NULL, "Failed to alloc ai array.");
    memcpy (gs->ai, ai, num * sizeof (CellHack_decide_action));

    gs->names = calloc (num, sizeof (char*));
    check (gs->names != NULL, "Failed to alloc names array.");
    memcpy (gs->names, names, num * sizeof (char*));

    gs->cells = calloc (width * height, sizeof (Cell));
    check (gs->cells != NULL, "Failed to alloc playing field.");

    gs->width  = width;
    gs->height = height;

    int i, j, di, dj, ei, ej, n;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            for (dj = -1, n = 0; dj <= 1; dj++) {
                for (di = -1; di <= 1; di++, n++) {
                    ei = (i + di) % width;
                    ej = (j + dj) % height;
                    if (ei < 0) ei = width - 1;
                    if (ej < 0) ej = height - 1;
                    gs->cells [i + width * j].neighbours [n] =
                         (gs->cells + ei + ej * width);
                }
            }
        }
    }

    int w_step = (int) floorf ((float) width  / side_length);
    int h_step = (int) floorf ((float) height / side_length);
    int idx;
    for (i = 0, n = 0; i < side_length; i++) {
        if (n >= num) break;
        for (j = 0; j < side_length; j++, n++) {
            if (n >= num) break;
            idx =  i * w_step + w_step / 2
                + (j * h_step + h_step / 2) * width;
            gs->cells [idx].type = n + 1;
            gs->cells [idx].energy = 100;
        }
    }

    return gs;
error:
    CellHack_destroy (gs);
    return NULL;
}

void
CellHack_destroy (GameState *gs)
{
    if (!gs) return;
    if (gs->cells) free (gs->cells);
    if (gs->names) free (gs->names);
    if (gs->ai)    free (gs->ai);
    if (gs)        free (gs);
}

void
CellHack_tick (GameState *gs)
{
    unsigned int temp, max_cells = gs->width * gs->height;
    unsigned int queue [max_cells];

    check (gs != NULL, "Got NULL as game state.");
    gs->turns += 1;

    uint8_t action, action_base, action_dir, live_neighbours;
    Cell* cell = NULL;
    int n, i;
    for (i = 0; i < gs->width * gs->height; i++) {
        queue [i] = i;
        cell = gs->cells + i;
        if (cell->type == 0 || cell->type == 255) continue;

        live_neighbours = 0;
        for (n = 0; n < 9; n++) {
            cell->env [n] = cell->neighbours [n]->type;
            if (cell->env [n] != 0 || cell->env [n] == 255) {
                live_neighbours++;
            }
        }
        action = gs->ai [cell->type - 1] (cell->env, cell->energy, &(cell->memory));

        action_base = action / 0x10;
        action_dir  = action % 0x10;
        switch (action_base) {
            case 0: // basic actions (rest, die, nothing)

                switch (action_dir) {
                    case 1: // rest
                        cell->energy += (live_neighbours >= 3) ? 7 - 2 * live_neighbours
                                                              : 1;
                        if (cell->energy > 200) {
                            cell->energy = 200;
                        }
                        break;
                    case 2: // nothing
                        break;
                    case 3: // die
                        cell->type = 0;
                        break;
                    default:
                        goto invalid;
                }

            case 1: // eat

                if (action_dir >= 9) goto invalid;
                if (cell->env [action_dir] != 0 && cell->env [action_dir] != 255) {
                    cell->energy += 1;
                    cell->neighbours [action_dir]->energy -= 1;
                }
                break;

            case 2: // move
            case 3: // split
                // both actions' execution is deferred until after all others
                // are evaluated
                cell->deferred_action = action;
                break;

            case 4: // feed -- reverse eat

                if (action_dir >= 9) goto invalid;
                if (cell->env [action_dir] != 0 && cell->env [action_dir] != 255) {
                    cell->energy -= 1;
                    cell->neighbours [action_dir]->energy += 1;
                }
                break;

            default:
            invalid:
                log_info ("player %s: invalid command", gs->names [cell->type - 1]);
        }
    }

    while (max_cells > 0) {
        i = randint (max_cells);
        cell = gs->cells + queue [i];

        temp = queue [max_cells - 1];
        queue [max_cells - 1] = queue [i];
        queue [i] = temp;
        max_cells -= 1;

        if (cell->energy < 20) {
            cell->type = 0;
            continue;
        }

        action = cell->deferred_action;
        switch (action / 0x10) {
            case 0: // nothing left to do
                break;

            case 2: // move

                cell->deferred_action = 0;
                action_dir  = action % 0x10;

                if (action_dir >= 9) goto invalid;
                if (cell->env [action_dir] == 0) {
                    cell->neighbours [action_dir]->type = cell->type;
                    cell->neighbours [action_dir]->energy = cell->energy;
                    cell->neighbours [action_dir]->memory = cell->memory;
                    cell->type = 0;
                }
                break;

            case 3: // split

                cell->deferred_action = 0;
                action_dir  = action % 0x10;

                if (action_dir >= 9) goto invalid;
                if (cell->env [action_dir] == 0) {
                    cell->energy /= 2;
                    cell->neighbours [action_dir]->type = cell->type;
                    cell->neighbours [action_dir]->energy = cell->energy;
                    cell->neighbours [action_dir]->memory = cell->memory;
                }
                break;
        }

    }


error:
    return;
}
