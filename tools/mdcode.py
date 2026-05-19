#!/usr/bin/env python3
import sys

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} INFILE OUTFILE", file=sys.stderr)
    sys.exit(1)

in_cpp = False
with open(sys.argv[1]) as infile, open(sys.argv[2], "w") as outfile:
    for line in infile:
        if in_cpp:
            if line.rstrip() == "```":
                in_cpp = False
            else:
                outfile.write(line)
        elif line.rstrip() == "```cpp":
            in_cpp = True
