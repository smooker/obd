#!/bin/bash
cd "$(dirname "$0")"
gcc -Wall -O2 -o ds150e  ds150e.c  ds150e_lib.c && echo "OK: ds150e"  || echo "FAIL: ds150e"
gcc -Wall -O2 -o bt_test  bt_test.c  ds150e_lib.c && echo "OK: bt_test"  || echo "FAIL: bt_test"
gcc -Wall -O2 -o ecu_scan ecu_scan.c ds150e_lib.c && echo "OK: ecu_scan" || echo "FAIL: ecu_scan"
gcc -Wall -O2 -o decode_bin decode_bin.c         && echo "OK: decode_bin" || echo "FAIL: decode_bin"
