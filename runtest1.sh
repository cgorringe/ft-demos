#!/usr/bin/env bash

set -e
G=-g25x20

while :; do
./plasma  $G -t5 &
./nb-logo $G -t5
../client/send-text $G -f fonts/5x8.bdf -o -l6 Welcome to Noisebridge &
./blur    $G -t5 bolt
./blur    $G -t5 boxes
./fractal $G -t5
./hack    $G -r1 hack
./lines   $G -t5 two
./quilt   $G -t5
done
