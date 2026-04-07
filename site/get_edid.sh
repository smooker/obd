#!/bin/bash
# get_edid.sh — extract EDID from X server for connected outputs
# Run as user with X access. Requires: xrandr, xxd, edid-decode (optional).
set -e
: "${DISPLAY:=:0}"
export DISPLAY

OUT=${1:-/tmp/edid}
mkdir -p "$OUT"

xrandr --props --verbose | awk -v out="$OUT" '
  /^[A-Za-z0-9_-]+ connected/ { conn=$1; f=0 }
  /EDID:/ { f=1; hex=""; next }
  f && /^[[:space:]]+[0-9a-f][0-9a-f]/ {
    gsub(/[[:space:]]/,"")
    hex=hex $0
    next
  }
  f && hex != "" {
    file=out "/" conn ".hex"
    print hex > file
    close(file)
    print "wrote", file
    f=0; hex=""
  }
'

for hex in "$OUT"/*.hex; do
  [ -s "$hex" ] || continue
  bin="${hex%.hex}.bin"
  xxd -r -p "$hex" > "$bin"
  echo
  echo "=== $(basename "$bin") ==="
  if command -v edid-decode >/dev/null; then
    edid-decode "$bin" | sed -n '1,40p'
    echo "  --- modes ---"
    edid-decode "$bin" | grep -E 'DTD|Detailed Timing|x[0-9]+ @' | head -20
  else
    echo "edid-decode not installed; raw bytes saved to $bin"
    head -c 128 "$bin" | xxd | head
  fi
done
