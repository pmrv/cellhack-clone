// Copyright 2015 Marvin Poul
// Licensed under the Do What The Fuck You Want To License, Version 2
// See LICENSE for details or http://www.wtfpl.net/txt/copying

#include <dlfcn.h>

#ifndef HEADLESS
#include <SDL2/SDL.h>
#endif

#include "cellhack/cellhack.h"

#define usage() fprintf (stderr, "USAGE: cellhack turns width height replay_file player_name path_to_ai_so … …")

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
 * returns 1 on receiving a QuitEvent, 0 otherwise
 */
int
gfx_display_cells (VideoState *vs, GameState *gs)
{
    SDL_Rect rect = {0};
    SDL_Event event = {0};
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

    while (SDL_PollEvent (&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return 1;
                break;
            default:
                break;
        }
    }

    SDL_Delay (17);
    return 0;
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
#endif

/* Init saving stuff
 * target_file      where to write the header
 * width, height    size of the playing field
 * max_players      number of players on the field
 * player_names     their names in the order that corresponds to the values of
 *                  Cell.type value (player_names [0] -> Cell.type = 1, …)
 *
 * format:
 *  > width: integer
 *  > height: integer
 *  > players: first_player, second_player, …
 *  > \000
 * this is followed by frames of 2 * width * height bytes for each turn
 * frames are as described in save_cells and no delimiter is between them
 */
FILE *
save_init (FILE *target_file, int width, int height, int max_players, char **player_names)
{
    int i;

    fprintf (target_file, "width: %i\n", width);
    fprintf (target_file, "height: %i\n", height);
    fprintf (target_file, "players: ");
    for (i = 0; i < max_players - 1; i++) {
        fprintf (target_file, "%s, ", player_names [i]);
    }
    fprintf (target_file, "%s\n", player_names [max_players - 1]);
    fwrite ("\0", 1, sizeof (char), target_file);

    return target_file;
}

/* Clean up saving stuff
 */
void
save_destroy (FILE *target_file)
{
    // we could write some footer here, but eh, don't see the need yet
    fclose (target_file);
}

/* Save type and energy of all cells directly into a file
 * target_file  where to write the data (assumes that header data was already
 *              written to that location
 * max_cells    number of cells in the next array
 * cells        pointer to an array of cells to save
 */
int
save_cells (FILE *target_file, int max_cells, Cell *cells)
{
    int i, ret;
    Cell cell;
    uint16_t buf[max_cells];
    for (i = 0; i < max_cells; i++) {
        cell = cells [i];
        buf [i] = cell.type << 8 | cell.energy;
    }
    ret = fwrite (buf, sizeof (uint16_t), max_cells, target_file);
    check (ret == max_cells, "Failed to write cell state to file.");

    return 0;

error:
    return 1;
}

int
main (int argc, char** argv)
{
    int i = 0, j, n = (argc - 4) / 2;
    int turns, width, height;
    char* names [n];
    void* dlls [n];
    CellHack_decide_action ais [n];
    GameState *gs = NULL;
    FILE *target_file = NULL;

    if (argc < 7 || argc % 2 == 0) {
        usage ();
        return 1;
    }

    turns = atoi (argv [1]);
    width  = atoi (argv [2]);
    height = atoi (argv [3]);

    for (i = 0; i < n; i += 1) {
        names [i] = argv [2 * i + 5];
        dlls [i]  = dlopen (argv [2 * i + 6], RTLD_LAZY);
        check (dlls [i] != NULL, "Failed to load dll for player '%s': %s",
               names [i], dlerror ());
        ais [i]   = dlsym (dlls [i], "cell_decide_action");
        check (ais [i] != NULL, "Failed to load ai function for player '%s': %s",
               names [i], dlerror ());
    }

    gs = CellHack_init (width, height, i, 1, ais, names);
    check (gs != NULL, "Failed to init CellHack.");

    target_file = fopen (argv [4], "w");
    check (target_file != NULL, "Failed to open replay file");
    save_init (target_file, width, height, i, names);

#ifndef HEADLESS
    int ret = 0;
    VideoState *vs = NULL;
    vs = gfx_display_init (i, width, height);
    check (vs != NULL, "Failed to init video state.");
#endif

    do {
#ifndef HEADLESS
        ret = gfx_display_cells (vs, gs);
        if (ret == 1) break;
#endif
        save_cells (target_file, Cellhack_width (gs) * Cellhack_height(gs), gs->cells);
        CellHack_tick (gs);
    } while (Cellhack_turns(gs) < turns);

#ifndef HEADLESS
    gfx_display_destroy (vs);
#endif

    CellHack_destroy (gs);
    // just for completeness sake, in the future we might decide to write a
    // footer or not use stdin
    save_destroy (target_file);

    return 0;

error:
    for (j = 0; j < i; j++) {
        dlclose (dlls [j]);
    }
    if (gs) CellHack_destroy (gs);
    if (target_file) fclose (target_file);
    return 1;
}
