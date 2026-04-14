#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -O2 -o ds150e ds150e.c ds150e_lib.c && echo "OK: ds150e" || echo "FAIL: ds150e"
