#!/usr/bin/env bash
# Build the XOR demo. Run from the repo root: ./build.sh
set -e
gcc -O2 -Wall -Wextra -I. examples/xor.c -o kestrel -lm
echo "built ./kestrel"
