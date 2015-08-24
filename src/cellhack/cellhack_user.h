#include <stdint.h>

// This header file is automatically included when you compile your code.

// Every cell is required to implement the following function:

// uint8_t cell_decide_action(uint8_t *env, int energy, uint64_t *memory)
//
// Called once for each of your cells, every frame of the game.
//
// Takes as input the cell's immediate environment, its energy, and its
// persistent memory.
// As output it should return a valid action, and optionally the stored
// memory can be changed.
//
// env - A description of the cell's immediate surroundings.
//       The environment is 9 bytes (a 3x3 region centered on cell).
//
//       0  1  2
//                      The values are 0 (EMPTY), 255 (ROCK),
//       3  4  5        or 1 - 4 indicating a cell from one of the 4 players.
//                      So env[4] is always just the player id of this cell.
//       6  7  8
//
// energy - The energy level of this cell.
//
// memory - The tiny persistent memory of this cell. Write to this 64bit
//          store to preserve some information between turns. If a cell
//          splits (reproduces) the memory will be duplicated.

uint8_t cell_decide_action(uint8_t *env, uint8_t energy, uint64_t *memory);


// named constants for environment offsets
#define LEFT_UP    0
#define UP_LEFT    0
#define UP         1
#define RIGHT_UP   2
#define UP_RIGHT   2
#define LEFT       3
#define CENTER     4
#define RIGHT      5
#define LEFT_DOWN  6
#define DOWN_LEFT  6
#define DOWN       7
#define RIGHT_DOWN 8
#define DOWN_RIGHT 8

// environment constants
#define EMPTY 0
#define ROCK 255

// Some helpful macros for testing the environment

#define isEmpty(dir) (env[dir]==EMPTY)

#define isFamily(dir) (env[dir]==env[CENTER])

#define isEnemy(dir)  \
    (env[dir]!=env[CENTER] && env[dir]!=EMPTY && env[dir]!=ROCK)

// -- Action enumeration --
// The rough idea is that the more crowded a cells environment is
// the more energy it takes per turn.
//
// Rest - When resting a cell will take in energy from the nutrient rich
//      solution around it; the more empty space in the cells environment the
//      more energy it gains from this.
// Nothing - Literally nothing. If an action like MOVE_UP fails, this is the
//      fall back move.
// Die - Cell energy immediately drops to zero.
//
// Actions that require a direction:
// NOTE, only cardinal directions are valid
//
// Eat - Takes energy from a neighboring cell. What kind of cell does not
//      matter; eating at your own cells is allowed.
// Move - Move cell into an empty space.
// Split - Cell splits into an empty space, creating two duplicate children
//      with half the energy of the parent. Cell memory of the children is
//      initially the value of the parent at the moment of splitting.
// Feed - Gives away some energy to a neighboring cell. What kind of cell
//      does not matter.

#define REST        1
#define NOTHING     2  //like rest, but no benefits
#define DIE         3

#define EAT_BASE    0x10 //not a valid action, needs direction
#define EAT_UP      (EAT_BASE+UP)
#define EAT_DOWN    (EAT_BASE+DOWN)
#define EAT_LEFT    (EAT_BASE+LEFT)
#define EAT_RIGHT   (EAT_BASE+RIGHT)

#define MOVE_BASE   0x20 //not a valid action, needs direction
#define MOVE_UP     (MOVE_BASE+UP)
#define MOVE_DOWN   (MOVE_BASE+DOWN)
#define MOVE_LEFT   (MOVE_BASE+LEFT)
#define MOVE_RIGHT  (MOVE_BASE+RIGHT)

#define SPLIT_BASE  0x30 //not a valid action, needs direction
#define SPLIT_UP    (SPLIT_BASE+UP)
#define SPLIT_DOWN  (SPLIT_BASE+DOWN)
#define SPLIT_LEFT  (SPLIT_BASE+LEFT)
#define SPLIT_RIGHT (SPLIT_BASE+RIGHT)

#define FEED_BASE  0x40 //not a valid action, needs direction
#define FEED_UP    (FEED_BASE+UP)
#define FEED_DOWN  (FEED_BASE+DOWN)
#define FEED_LEFT  (FEED_BASE+LEFT)
#define FEED_RIGHT (FEED_BASE+RIGHT)


// Some energy settings in the game
#define ENERGY_MIN        20 //cell dies below this
#define ENERGY_MIN_SPLIT  40 //needs to be larger than this to split
#define ENERGY_MAX       200 //energy clipped to this amount
#define ENERGY_START     100 //only used for initial cell

