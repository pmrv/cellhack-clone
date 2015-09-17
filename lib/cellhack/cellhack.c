// Copyright 2015 Marvin Poul
// Licensed under the Do What The Fuck You Want To License, Version 2
// See LICENSE for details or http://www.wtfpl.net/txt/copying

#include "cellhack.h"

void *
Executor (ExecutorArgs *args)
{
    Cell *cell;
    int err = 0;
    uint8_t result = 0;
    check (args != NULL, "Got NULL as args.");
    err = pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    check (err == 0, "Failed to set cancel type to asynchronous.");

    while (1) {

        pthread_barrier_wait (&args->barrier);

        pthread_mutex_lock (&args->lock);

        cell = args->arg_cell;

        pthread_mutex_unlock (&args->lock);

        // mutex must be unlocked here otherwise the main thread cannot cancel
        // if the user function unexpectedly blocks (because it cannot wake up
        // from the condition wait).
        result = args->work (cell->env, cell->energy, &cell->memory);

        pthread_mutex_lock (&args->lock);

        args->result = result;
        args->done = 1;
        pthread_cond_signal (&args->cond);

        pthread_mutex_unlock (&args->lock);

    }

error:
    return NULL;
}


/* returns an integer from 0 to max exclusive */
unsigned int
randint (unsigned int max)
{
    return rand () % max; // can't be arsed about modulo bias just yet
}


GameState*
CellHack_init (int width, int height, int num, unsigned int timeout,
               CellHack_decide_action* ai, char** names)
{
    GameState *gs = NULL;
    int err = 0;
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

    gs->ea = calloc (1, sizeof (ExecutorArgs));
    check (gs->ea != NULL, "Failed to alloc executor arguments.");

    err = pthread_mutex_init (&gs->ea->lock, NULL);
    check (err == 0, "Failed to init mutex.");

    pthread_condattr_t cond_attr;
    pthread_condattr_init (&cond_attr);
    pthread_condattr_setclock (&cond_attr, CLOCK_REALTIME);
    err = pthread_cond_init (&gs->ea->cond, &cond_attr);
    check (err == 0, "Failed to init condition.");

    err = pthread_barrier_init (&gs->ea->barrier, NULL, 2);
    check (err == 0, "Failt to init barrier.");

    err = pthread_create (&gs->etid, NULL, (void *(*)(void *)) Executor, gs->ea);
    check (err == 0, "Failed to create executor thread.");

    gs->width  = width;
    gs->height = height;
    gs->timeout = timeout;

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
    int err;
    struct timespec ts;
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

        pthread_mutex_lock (&gs->ea->lock);

        gs->ea->arg_cell = cell;
        gs->ea->work = gs->ai [cell->type - 1];

        err = clock_gettime (CLOCK_REALTIME, &ts);
        check (err == 0, "Failed to get clock time, bailing.");

        ts.tv_sec += gs->timeout;

        pthread_barrier_wait (&gs->ea->barrier);

        do {
            err = pthread_cond_timedwait (&gs->ea->cond, &gs->ea->lock, &ts);
        } while (gs->ea->done == 0 && err == 0);

        if (err == ETIMEDOUT) {
            log_info ("Player %s timed out.", gs->names [cell->type - 1]);

            pthread_cancel (gs->etid);
            pthread_mutex_unlock (&gs->ea->lock);

            err = pthread_create (&(gs->etid), NULL, (void *(*)(void *)) Executor, gs->ea);
            check (err == 0, "Failed to recreate executor thread.");

            action = 2;
        } else
        if (err == 0) {
            action = gs->ea->result;
            gs->ea->done = 0;

            pthread_mutex_unlock (&gs->ea->lock);
        } else {
            sentinel ("Error while waiting on condition.");
        }

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
