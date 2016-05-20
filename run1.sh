#!/usr/bin/env bash

echo "Running Demo Set #1..."

set -e
G=-g45x35

while :; do

  ../client/send-text -g45x5 -l6  -f fonts/5x5.bdf -o Welcome to Noisebridge &
  ./plasma  $G -l1 -t10
  ./nb-logo $G -l5 -t10 &
  ./blur    $G -l1 -t10 bolt
  ./hack    $G -l4 -r1 hack &
  ./blur    $G -l1 -t10 boxes
  ./fractal $G -l1 -t10
  ./lines   $G -l2 -t10 two
  ./quilt   $G -l3 -t10
  # ./black   $G all

done
