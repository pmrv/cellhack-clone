// Copyright 2015 Marvin Poul
// Licensed under the Do What The Fuck You Want To License, Version 2
// See LICENSE for details or http://www.wtfpl.net/txt/copying

#include <dlfcn.h>

#ifndef HEADLESS
#include <SDL2/SDL.h>
#endif

#include "cellhack/cellhack.h"

#define usage() fprintf (stderr, "USAGE: cellhack turns width height player_name path_to_ai_so … …")

#ifndef HEADLESS
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width;
    int height;
    int cell_width;
    int cell_height;
    uint8_t *colors;
} VideoState;

/* Initialize graphics stuff
 * num      number of players in the game
 * width    width of the game field in cells
 * height   height of the game field in cells
 */
VideoState *
gfx_display_init (int num, int width, int height)
{
    VideoState *vs = NULL;
    int err = 0, i, n;

    vs = calloc (1, sizeof (VideoState));
    check (vs != NULL, "Failed to alloc video state.");
    vs->colors = calloc (3 * num, sizeof (uint8_t));
    check (vs->colors != NULL, "Failed alloc colors array.");

    // TODO: make window size configurable
    vs->width       = 800;
    vs->height      = 600;
    vs->cell_width  = vs->width / width;
    vs->cell_height = vs->height / height;

    for (i = 0; i < num; i++) {
        for (n = 0; n < 3; n++) {
            vs->colors [i + num * n] = rand () % 256;
        }
    }

    err = SDL_CreateWindowAndRenderer (vs->width, vs->height, 0,
                                       &(vs->window), &(vs->renderer));
    check (err == 0, "Failed to create window and renderer.");

    return vs;

error:
    if (vs && vs->colors) free (vs->colors);
    if (vs) free (vs);
    return NULL;
}

/* Update window to show current cells
 */
void
gfx_display_cells (VideoState *vs, GameState *gs)
{
    SDL_Rect rect = {0};
    rect.w = vs->cell_width;
    rect.h = vs->cell_height;
    Cell *cell = NULL;
    int width, height, x, y;
    uint8_t *colors;

    SDL_SetRenderDrawColor (vs->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear (vs->renderer);

    SDL_SetRenderDrawColor (vs->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    width = Cellhack_width (gs);
    height = Cellhack_height (gs);
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            cell = gs->cells + x + y * width;
            if (cell->type == 0) continue;

            rect.x = x * vs->cell_width;
            rect.y = y * vs->cell_height;

            colors = vs->colors + (cell->type - 1);
            SDL_SetRenderDrawColor (vs->renderer, colors [0], colors [1], colors [2],
                                    cell->energy + 55);
            SDL_RenderFillRect (vs->renderer, &rect);
        }
    }

    SDL_RenderPresent (vs->renderer);
    SDL_Delay (100);
}

/* Destroy graphics stuff
 */
void
gfx_display_destroy (VideoState *vs)
{
    if (vs == NULL) return;

    SDL_DestroyRenderer (vs->renderer);
    SDL_DestroyWindow (vs->window);

    free (vs);
}
#else
void
txt_display_cells (GameState* gs)
{
    check (gs != NULL, "Got NULL as game state.");

    int i;
    for (i = 0; i < Cellhack_width(gs) * Cellhack_height(gs); i++) {
        printf ("%i", gs->cells [i].type);
        if (i > 0 && (i + 1) % Cellhack_width(gs) == 0) printf ("\n");
    }
    printf ("\n");

error:
    return;
}
#endif

int
main (int argc, char** argv)
{

    int i = 0, j, n = (argc - 3) / 2;
    int turns, width, height;
    char* names [n];
    void* dlls [n];
    CellHack_decide_action ais [n];
    GameState *gs = NULL;

    if (argc < 4 || argc % 2 == 1) {
        usage ();
        return 1;
    }

    turns = atoi (argv [1]);
    width  = atoi (argv [2]);
    height = atoi (argv [3]);

    for (i = 0; i < n; i += 1) {
        names [i] = argv [2 * i + 4];
        dlls [i]  = dlopen (argv [2 * i + 5], RTLD_LAZY);
        check (dlls [i] != NULL, "Failed to load dll for player '%s': %s",
               names [i], dlerror ());
        ais [i]   = dlsym (dlls [i], "cell_decide_action");
        check (ais [i] != NULL, "Failed to load ai function for player '%s': %s",
               names [i], dlerror ());
    }

    gs = CellHack_init (width, height, i, ais, names);
    check (gs != NULL, "Failed to init CellHack.");

#ifndef HEADLESS
    VideoState *vs = NULL;
    vs = gfx_display_init (i, width, height);
    check (vs != NULL, "Failed to init video state.");
#endif

    do {
#ifndef HEADLESS
        gfx_display_cells (vs, gs);
#else
        txt_display_cells (gs);
#endif
        CellHack_tick (gs);
    } while (Cellhack_turns(gs) < turns);


#ifndef HEADLESS
    gfx_display_destroy (vs);
#endif

    CellHack_destroy (gs);

    return 0;

error:
    for (j = 0; j < i; j++) {
        dlclose (dlls [j]);
    }
    if (gs) CellHack_destroy (gs);
    return 1;
}
