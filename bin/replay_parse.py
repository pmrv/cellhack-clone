#!/usr/bin/env python3
import json
import sys

def replay_parse (ifile, ofile = sys.stdout):
    dump = ifile.read ()
    split_idx = dump.find (b'\n\0')
    txt = dump [:split_idx].decode ("utf8")
    raw = dump [split_idx + 2:]

    meta_data = {}
    for k, v in map (lambda x: x.split (":"), txt.split ("\n")):
        meta_data [k] = v.strip ()

    try:
        width = int (meta_data ["width"])
        height = int (meta_data ["height"])
        players = ["dead"] + list (map (lambda x: x.strip (),
                        meta_data ["players"].split (',')))
    except KeyError:
        print ("replay file lacks some meta data, bailing.")
        sys.exit (1)

    cell_states = []
    while raw:
        frame = raw [:width * height * 4]
        raw = raw [4 * width * height:]

        for j in range (height):
            cell_states.append ([])
            for i in range (width):
                idx  = 2 * (i + j * width)
                cell = frame [idx:idx + 2]
                cell_states [-1].append (
                        {"player": players [int (cell [1])],
                        "energy": int (cell [0])})

    json.dump ({"meta": meta_data, "frames": cell_states}, ofile)

def main ():
    with open (sys.argv [1], "rb") as ifile:
        if len (sys.argv) < 3:
            replay_parse (ifile)
        else:
            with open (sys.argv [2], "w") as ofile:
                replay_parse (ifile, ofile)


if __name__ == "__main__": main ()
