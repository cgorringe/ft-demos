#!/usr/bin/env bash

echo "Running Demo..."

# export FT_DISPLAY=10.20.0.40
# cd `dirname $0`
export FT_DISPLAY=localhost

set -e
G1=-g45x35
G2=-g35x25+5+5
F1=fonts/5x5.bdf

./plasma $G1 -l1  &

while :; do

  # plasma
  sleep 10
  ./send-text -g45x5+0+0 -l13 -b010101 -f $F1 -o "Welcome to STEAM Vault" &
  sleep 50

  # blur + hack
  ./blur    $G1 -l2 -t75 bolt &
  sleep 10
  ./send-text -g45x5+0+0 -l13 -f $F1 -o "Welcome Hayward" &
  sleep 40
  ./hack    $G1 -l4 -r1  steam &
  ./send-text -g45x5+0+0 -l13 -cFF00FF -f $F1 -o "STEAM" &
  sleep 5
  ./send-text -g45x5+0+30 -l14 -f $F1 -o "science tech engineering art multimedia" &
  sleep 20
  ./blur    $G1 -l3 -t30 boxes &
  sleep 30

  # plasma
  sleep 30

  # fractal
  ./fractal $G1 -l2 -t90 &
  sleep 40
  ./send-text -g45x5+0+0 -l14 -f $F1 -o "STEAM Vault" &
  sleep 50

  # plasma
  sleep 30

  # lines
  ./black   $G2 -l3 -t50 -b &
  sleep 1
  ./lines   $G2 -l6 -t48 two &
  sleep 29
  ./send-text -g45x5+0+0 -l13 -f $F1  -o "Be Excellent to Each Other" &
  sleep 30
#  ./black   $G2 -l3

done
