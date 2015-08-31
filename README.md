Cell Hack Clone
=

Cell Hack is a game where you control the behaviour of a number of cells with
the aim to either outnumber or wipe out all other players in an arena. Cells
have a certain amount of health (called `energy`, starts at 100 and is capped at 256), they can eat (take energy)
from or feed (give energy) to other cells, move or split themselves. All
actions happen in turns. If their energy falls below a certain threshold in a
round they die. To pull the strings you provide a function in plain C that is
called for all of your cells and is supposed to return an action, e.g. "move to
the left" or "eat cell to the right" encoded in a byte. This is the function's
type

```
uint8_t cell_decide_action(uint8_t *env, uint8_t energy, uint64_t *memory);
```

where `env` is an array of the nine cells in your immediate surroundings
(including yourself), `energy` is the life energy of the current cell and
`memory` a pointer to four bytes of persistent memory each cell has.

Mechanics
-

Your cell has the following actions at its disposal
* nothing; it simply waits, fall-back if e.g. your function fails to
  return after a given time or returns an invalid move
* die; well, duh
* rest; restores some of it energy. The current formula is `r =
  max (7 - 2 * n, 1)`, where `r` is the return energy and `n` is the number of
  neighbours it has. The idea is that the denser cells
  are packed, the less energy they gain by resting.
* eat; if there is any cell in the target, reduce its energy by `k` and
  increase yours by `k`, currently `k = 1`
* feed; basically "eat" but reversed with you loosing energy
* move; if the target is blocked by another cell, nothing happens
* split; Create a new cell in the target with half your energy and a copy
  of your memory. Nothing happens if the target is occupied.
Macros for the values these actions are defined in [a separate
header](src/cellhack/player.h) that I took directly from https://cellhack.net.
All actions that reference a target must be combined with a direction, either
"up", "down", "left" or "right", again macros exist for those.
Once you're done, simply return the value.

Execution order of the cell is (or should appear to be) as follows
* rest
* eat, feed
* all others

The order in which "move" and "split" actions (which may block each other) are
acted upon is random. This means that currently e.g. an enemy cell can keep you
from splitting, kill you before you move away or you cannot deny your own cells
to prevent an enemy feeding on them.

The winner is the player with the most cells after a number of turns.

All of this subject to change for the sake of balancing.

Implementation
-

Build with (you guessed it) `make`, it should™ just work. The default build
comes with a simple SDL interface, if you don't want that or don't have SDL2
installed, build with `make OPTFLAGS=-DHEADLESS`.
Run with `./build/cellhack number_of_turns width height player_name
path_to_shared_object … …`, `width` and `height` give the size of the arena
(with warped edges), `turns` how long the game should be played `player_name`
is the name of the first player and `path_to_shared_object` a path from where
their cell function can be loaded. You can repeat the last two up to 254 times,
this will just add more players. The game will give all players a single
starter cell and will try to arrange them equidistant and square shaped in the
arena, if the number of players is not a square number, some places on one edge
of the square will be left empty.

Build the shared objects with `$(CC) --shared -Isrc -o some_path.so
some_other_path.c` from the repository root.
