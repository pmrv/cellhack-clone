#include "display.h"

void
display_cells (GameState* gs)
{
    check (gs != NULL, "Got NULL as game state.");

    int i;
    for (i = 0; i < gs->width * gs->height; i++) {
        printf ("%i", gs->cells [i].type);
        if (i > 0 && (i + 1) % gs->width == 0) printf ("\n");
    }
    printf ("\n");

error:
    return;
}
