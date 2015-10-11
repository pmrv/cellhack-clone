#include "cellhack/player.h"
#include <stdlib.h>

static uint8_t action_list[] = {
    MOVE_UP,MOVE_DOWN,MOVE_LEFT,MOVE_RIGHT,
    SPLIT_UP,SPLIT_DOWN,SPLIT_LEFT,SPLIT_RIGHT };
static int init=0;

uint8_t cell_decide_action(uint8_t *env, uint8_t energy, uint64_t *memory)
{
    if(!init)
    {
        srand(env[CENTER]); //seed based on our player number
        init=1;
    }

    //if low on energy, rest
    if(energy<100)
        return REST;

    //otherwise choose a useful action
    return action_list[(rand()%sizeof(action_list))];
}
