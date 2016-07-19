#!/usr/bin/env bash

#export FT_DISPLAY=10.20.0.40

echo "Running Demo..."

cd `dirname $0`
set -e
G1=-g45x35
G2=-g35x25+5+5

../plasma  $G1 -l1  &

while :; do

  # plasma
  sleep 10
  ../send-text -g45x5+0+0 -l13 -b010101 -f fonts/5x5.bdf -o "Welcome to Noisebridge" &
  sleep 10
  ../send-text -g45x5+0+30 -l14 -cFF0000 -b010101 -f fonts/5x5.bdf -o "noisebridge.net" &
  sleep 10
  ../nb-logo $G2 -l5 -t28 &
  ../send-text -g45x5+0+0 -l13 -f fonts/5x5.bdf -o "open to the public in SF" &
  sleep 30

  # blur + hack
  ../blur    $G1 -l2 -t30 bolt &
  ../send-text -g45x5+0+0 -l14 -o "Circuit Hacking Mondays" &
  sleep 10
  ../hack    $G1 -l4 -r1  hack &
  ../send-text -g45x5+0+0 -l13 -cFF00FF -o "Hack the Planet" &
  sleep 10
  ../send-text -g45x5+0+30 -l14 -o "classes electronics coding woodshop sewing 3D-printers community" &
  sleep 10
  ../blur    $G1 -l3 -t20 boxes &
  sleep 20

  # plasma
  ../send-text -g45x5+0+0 -l13 -o "Open to the public" &
  sleep 20

  # fractal
  ../fractal $G1 -l2 -t70 &
  ../send-text -g45x5+0+0 -l14 -o "Be Excellent to Each Other" &
  sleep 20
  ../send-text -g45x5+0+30 -l13 -o "classes electronics coding woodshop sewing 3D-printers community" &
  sleep 30
  ../send-text -g45x5+0+0 -l14 -o "Welcome to Noisebridge" &
  sleep 20

  # scrolling logo
  #../send-image ~/images/nblogo.png -s40 -l4 -t10
  sleep 10

  # plasma
  sleep 10

  # lines
  ../black   $G2 -l3 -t65 -b &
  sleep 1
  ../lines   $G2 -l6 -t38 two &
  sleep 9
  ../send-text -g45x5+0+0 -l13 -cFF0000 -b010101 -o "noisebridge.net" &
  sleep 10
  ../send-text -g45x5+0+30 -l14 -b010101 -o "In San Francisco" &
  sleep 20

  # quilt
  ../quilt   $G2 -l7 -t30 &
  sleep 15
  ../send-text -g45x5+0+0 -l13 -o "Be Excellent to Each Other" &
  sleep 15
  ../black   $G2 -l7

done
