#!/usr/bin/env bash

#export FT_DISPLAY=10.20.0.40

echo "Running Demo..."
echo "This runs most demos on layers 0-2"

cd `dirname $0`
set -e
G1=-g45x35
G2=-g35x25+5+5
F1=../fonts/5x5.bdf

while :; do

  # plasma
  ../plasma  $G1 -l0 -t180 &
  sleep 60
  ../send-text -g45x5+0+0 -l1 -b010101 -f $F1 -o "Welcome to Noisebridge" &
  sleep 120

  # blur + hack
  ../blur $G1 -l0 -t60 bolt &
  sleep 30
  ../send-text -g45x5+0+0 -l1 -cFF00FF -f $F1 -o "Hack the Planet" &
  sleep 30
  ../blur $G1 -l0 -t60 boxes &
  sleep 60

  # fractal
  ../fractal $G1 -l0 -t120 &
  sleep 30
  ../send-text -g45x5+0+0 -l1 -f $F1 -o "Be Excellent to Each Other" &
  sleep 90

  # plasma
  ../plasma  $G1 -l0 -t95 &
  sleep 30

  # lines
  ../black $G2 -l1 -t65 -b &
  sleep 1
  ../lines $G2 -l2 -t38 two &
  sleep 39

  # quilt
  ../quilt $G2 -l2 -t30 &
  sleep 30
  ../black $G2 -l1

  # game of life
  ../life $G1 -l0 -b0 -r30 -t125 &
  sleep 30
  ../send-text -g45x5+0+0 -l1 -f $F1 -o "Be Excellent to Each Other" &
  sleep 90

done
