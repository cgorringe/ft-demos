// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// midi
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/12/2016
//
// Displays a player piano based on midi input.
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./midi localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./midi
//
// --------------------------------------------------------------------------------
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>


// random int in range min to max inclusive
int randomInt(int min, int max) {
  //return (arc4random_uniform(max - min + 1) + min);
  return (random() % (max - min + 1) + min);
}

void outputNoteOff(uint8_t note, FILE * stream) {

    uint8_t bytes[3];
    bytes[0] = 0x80;
    bytes[1] = (note & 0x7F);
    bytes[2] = 64;  // velocity
    if ( fwrite(&bytes, 3, 1, stream) ) {
        fprintf(stderr, "error0 ");  // shouldn't get here
    }
}

void outputNoteOn(uint8_t note, FILE * stream) {

    uint8_t bytes[3];
    bytes[0] = 0x90;
    bytes[1] = (note & 0x7F);
    bytes[2] = 64;  // velocity
    if ( fwrite(&bytes, 3, 1, stream) ) {
        fprintf(stderr, "error1 ");  // shouldn't get here
    }
}


// --------------------------------------------------------------------------------
// TESTS

void test1() {

    static uint8_t old_note=0, new_note=60;
    static int dir=1;

    outputNoteOff(old_note, stdout);
    outputNoteOn(new_note, stdout);

    old_note = new_note;
    new_note += dir;
    if (new_note > 80) { dir = -1; }
    if (new_note < 40) { dir = +1; }
}

void test2() {

    static uint8_t old_note=0;
    uint8_t new_note = randomInt(50, 70);

    outputNoteOff(old_note, stdout);
    outputNoteOn(new_note, stdout);

    old_note = new_note;
}


// --------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    srandom(time(NULL)); // seed the random generator

    while (1) {



        test1();
        //test2();

        sleep(1);
    }
}
