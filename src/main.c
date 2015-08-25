// Copyright 2015 Marvin Poul
// Licensed under the Do What The Fuck You Want To License, Version 2
// See LICENSE for details or http://www.wtfpl.net/txt/copying

#include <dlfcn.h>

#include "cellhack/cellhack.h"

#define usage() fprintf (stderr, "USAGE: cellhack turns width height player_name path_to_ai_so … …")

void
display_cells (GameState* gs)
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

    display_cells (gs);
    while (Cellhack_turns(gs) < turns) {
        CellHack_tick (gs);
        display_cells (gs);
    }

    CellHack_destroy (gs);

    return 0;

error:
    for (j = 0; j < i; j++) {
        dlclose (dlls [j]);
    }
    return 1;
}
