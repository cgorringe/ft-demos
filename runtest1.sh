#!/usr/bin/env bash

set -e

while :; do
./plasma  -g25x20 -t5 &
./nb-logo -g25x20 -t5
../client/send-text -g25x20 -f fonts/5x8.bdf -o -l6 Welcome to Noisebridge &
./blur    -g25x20 -t5 bolt
./blur    -g25x20 -t5 boxes
./fractal -g25x20 -t5
done
