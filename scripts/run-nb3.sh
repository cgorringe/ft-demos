#!/usr/bin/env bash

#export FT_DISPLAY=10.20.0.40
export FT_DISPLAY=127.0.0.1

echo "Running Demo using layers 1-3..."

cd `dirname $0`
set -e

# Old FT 9x7 crates
# G1=-g45x35
# G2=-g35x25+5+5
# New FT 9x8 crates
G1=-g45x40
G2=-g35x30+5+5
F1=../fonts/5x5.bdf

# plasma timesout in 30 days (60 * 60 * 24 * 30)
../plasma  $G1 -l1 -t2592000 &

while :; do

  # plasma
  sleep 60

  # matrix
  ../matrix $G1 -l3 -t40 &
  sleep 40

  # sierpinski over plasma
  ../sierpinski $G1 -l2 -c0 -r30 -t60 &
  sleep 60

  # blur
  ../blur $G1 -l3 -t40 bolt &
  sleep 40
  ../blur $G1 -l2 -t40 boxes &
  sleep 40
  ../blur $G1 -l3 -t40 circles &
  sleep 40

  # fractal
  ../fractal $G1 -l2 -t179 &
  sleep 120

  # maze over fractal
  ../maze $G1 -l3 -v0 -b0 -t60 &
  sleep 60

  # lines
  ../black $G2 -l2 -t65 -b &
  sleep 1
  ../lines $G2 -l3 -t38 two &
  sleep 39

  # quilt
  ../quilt $G2 -l3 -t30 &
  sleep 30
  # clears black box
  ../black $G2 -l2 &

  # game of life
  ../life $G1 -l2 -c0 -b0 -r30 -t120 &
  sleep 120

  # maze over plasma
  ../maze $G1 -l3 -v0 -b0 -t60 &
  sleep 60

  # Sierpinski rain over plasma (not working on Raspi yet)
  # ../sierpinski_rain.py -t 60 -x -c blacktransp &
  # sleep 60

done
