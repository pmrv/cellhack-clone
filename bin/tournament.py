#!/usr/bin/env python

import atexit
import subprocess
import sys
from argparse import ArgumentParser as AP

def pack (it, n):
    l = []
    k = 0
    for i in it:
        if k < n:
            l.append (i)
            k += 1
        else:
            yield l
            l = [i]
            k = 1
    else:
        yield l

if __name__ == "__main__":
    ap = AP ("tournament",
             description = "Tournament runner for cellhack programs.")
    ap.add_argument ("--replay-dir", "-r", default = None,
                     help = "Where to store match replays, if not given, do"
                            "not store replays")
    ap.add_argument ("--binary", "-b", default =  "./bin/cellhack",
                     help = "Path to the cellhack binary")
    ap.add_argument ("--group_size", "-g", default = 4, type = int,
                     help = "How many programs fight in one match.")
    ap.add_argument ("--turns", "-t", default = 2, type = int,
                     help = "How many turns each match should have.")
    ap.add_argument ("--size", "-s", default = (10, 10), nargs = 2, type = int,
                     metavar = ("X", "Y"),
                     help = "Size of the playing field.")
    ap.add_argument ("--input", "-i", default = sys.stdin, type = open,
                help = "From where to read the contentants of the turnament.")
    args = ap.parse_args ()
    atexit.register (lambda : args.input.close ())

    players = {}

    for line in args.input.readlines ():
        if not line.strip (): continue
        name, path = line.split ()
        players [name] = path

    generation = 1
    this_generation = list (players.keys ())
    while len (this_generation) > 1:
        next_generation = []
        match = 1
        for this_match in pack (this_generation, args.group_size):
            replay_file = args.replay_dir \
                        + "match-{}-generation-{}".format (match, generation) \
                          if args.replay_dir != None else "/dev/null"
            output = subprocess.check_output (
                    [args.binary, str (args.turns),
                     str (args.size [0]), str (args.size [1]),
                     replay_file] + sum (([n, players [n]]
                                            for n in this_match), []))

            outcome = []
            found = False
            for line in output.decode ().split ('\n'):
                if   line != "Player: Cells surviving" and found: continue
                elif line == "Player: Cells surviving": found = True; continue

                n, cs = line.split (":")
                outcome.append ( n, int (cs) )

            winner = max (outcome, key = lambda x: x [1])
            next_generation.append (winner)
            match += 1

        generation += 1
        this_generation = next_generation

    print (this_generation [0])
