#include "cellhack/player.h"
#include <stdlib.h>

uint8_t cell_decide_action (uint8_t *env, uint8_t energy,
                            uint64_t *memory)
{
    int cmd_type = random () % 5;
    int rand_cmd = 1 << (8 * cmd_type);
    if (cmd_type > 0) { // movement command
        rand_cmd += random () % 9;
    }

    return rand_cmd;
}
