#!/usr/bin/env bash

#export FT_DISPLAY=10.20.0.40

echo "Running Demo using layers 1-3..."

cd `dirname $0`
set -e
G1=-g45x35
G2=-g35x25+5+5
F1=../fonts/5x5.bdf

# plasma timesout in 30 days (60 * 60 * 24 * 30)
../plasma  $G1 -l1 -t2592000 &

while :; do

  # plasma
  sleep 120

  # blur + hack
  ../blur $G1 -l2 -t60 bolt &
  sleep 60
  ../blur $G1 -l3 -t60 boxes &
  sleep 60

  # fractal
  ../fractal $G1 -l2 -t120 &
  sleep 120

  # plasma
  sleep 30

  # maze over plasma
  ./maze -l3 -v0 -b0 -t45 &
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
  ./maze -l3 -v0 -b0 -t90 &
  sleep 90

done
