#!/usr/bin/env bash

echo "Running Demo..."

cd `dirname $0`
set -e
G=-g45x35

while :; do
../plasma  $G -l1 -t10 &
../nb-logo $G -l5 -t10
../send-text -g25x5 -l6  -f fonts/5x5.bdf -o Welcome to Noisebridge &
../blur    $G -l1 -t10 bolt
../hack    $G -l4 -r1 hack &
../blur    $G -l1 -t10 boxes
../fractal $G -l1 -t10
../lines   $G -l2 -t10 two
../quilt   $G -l3 -t10
done
