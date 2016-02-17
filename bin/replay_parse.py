#!/usr/bin/env python3
import itertools
import struct
import json
import sys

def parse_meta (buf):
    # last char is a newline, we don't want that
    txt = bytes(itertools.takewhile(lambda c: c != 0,
                (buf.read(1)[0] for _ in itertools.count()))).decode("utf8")[:-1]
    meta_data = {}
    for k, v in map (lambda x: x.split (":"), txt.split ("\n")):
        meta_data [k] = v.strip ()

    return meta_data

cell_struct = struct.Struct("=BBQ")
def parse_cell (buf, players):
    raw = buf.read (cell_struct.size)
    if len(raw) < cell_struct.size: raise EOFError
    player_id, energy, memory = cell_struct.unpack (raw)
    return {"player": players [player_id],
            "energy": energy,
            "memory": memory}

def parse_frames (buf, width, height, players):
    while 1:
        frame = []
        for _ in range (height):
            frame.append ([])
            for _ in range (width):
                try:
                    frame [-1].append (parse_cell (buf, players))
                except EOFError:
                    raise StopIteration

        yield frame

def parse (buf):
    meta_data = parse_meta (buf)

    try:
        width = int (meta_data ["width"])
        height = int (meta_data ["height"])
        players = ["dead"] + list (map (lambda x: x.strip (),
                        meta_data ["players"].split (',')))
    except KeyError:
        print ("replay file lacks some meta data, bailing.")
        sys.exit (1)

    frames = list(parse_frames(buf, width, height, players))
    return {"meta": meta_data, "frames": frames}

if __name__ == "__main__":
    with open (sys.argv [1], "rb") as ifile:
        if len (sys.argv) < 3:
            json.dump (parse (ifile), sys.stdout)
        else:
            with open (sys.argv [2], "w") as ofile:
                json.dump (parse (ifile), ofile)

