#include "cellhack/player.h"

// 1 -> 1
// 3 -> 2
// 5 -> 4
// 7 -> 8
#define transDir(i) (1 << (i) / 2)

uint8_t cell_decide_action (uint8_t *env, uint8_t energy,
                            uint64_t *memory)
{
    // mem [0] tracks current state:
    //  0: default
    //  1: moved in position to split
    //  2: just split
    // mem [1] tracks movement direction
    // mem [2] tracks in which direction the cell already split into
    int i;
    uint8_t *mem = (uint8_t *) memory;

    switch (mem [0]) {
        default:
            break;

        case 1:
            if (!isEmpty (mem [1])) {
                // direction we wanted to split into is already taken, remember
                // that and return
                mem [2] |= transDir (mem [1]);
                mem [0] = 0;
                return MOVE_BASE + 8 - mem [1];
            } else {
                mem [0] = 2;
                return SPLIT_BASE + mem [1];
            }
        case 2:
            mem [0] = 0;
            if (isEmpty (8 - mem [1])) {
                // parent cell
                // add direction to the directions we split into
                mem [2] |= transDir (mem [1]);
                return MOVE_BASE - mem [1] + 8;
            } else {
                // child cell
                // remember in which direction the parent was
                mem [2] = transDir (8 - mem [1]);
            }
    }

    if (energy < ENERGY_MIN_SPLIT + 10) goto rest;

    for (i = 1; i < 9; i += 2) {
        if (isEmpty (i) && (mem [2] & transDir (i)) == 0) {
            mem [0] = 1;
            mem [1] = i;
            return MOVE_BASE + i;
        }
    }

    for (i = 1; i < 9; i += 2) {
        if (isEnemy (i)) return EAT_BASE + i;
    }

rest:
    return REST;
}
