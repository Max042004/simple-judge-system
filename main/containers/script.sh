#!/bin/sh
set -e

SRC=/src/myprog.c   # inside‑container path
echo "==> Compiling $SRC"
gcc -O2 -o "$SRC"
exec /src/myprog.o