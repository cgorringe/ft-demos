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

#include "udp-flaschen-taschen.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
// #include <math.h>
#include <time.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 5       // (0-15) 0=background
#define DELAY 50


// random int in range min to max inclusive
int randomInt(int min, int max) {
  //return (arc4random_uniform(max - min + 1) + min);
  return (random() % (max - min + 1) + min);
}

void scrollUp(UDPFlaschenTaschen &canvas) {

    int width = canvas.width();
    int height = canvas.height();
    Color color;

    // scroll up by 1
    for (int y=1; y < height; y++) {
        for (int x=0; x < width; x++) {
            color = canvas.GetPixel(x, y);
            canvas.SetPixel(x, y-1, color);
        }
    }
    // don't clear last row
}

// notes[] is a byte array of length 128, where notes[60] is middle-C.
// each byte ranges 0-127 indicating note 'velocity', where 0 = note off, 1-127 = note on
// Only draws number of notes = to canvas width, centered on middle-C. Draws on last row.
// TODO: color palette

void drawNotes(uint8_t notes[], UDPFlaschenTaschen &canvas) {

    int width = canvas.width();
    int height = canvas.height();
    Color color;
    int v;

    if (width > 120) { return; } // never happens
    int i = 60 - (width >> 1);  // center on middle-C (60)

    for (int x=0; x < width; x++) {
        v = notes[i] * 2;
        color = Color(0, v, 0);  // TODO
        canvas.SetPixel(x, height-1, color);
        i++;
    }
}

// note = 0 to 127
void noteOn(uint8_t note, uint8_t velocity, uint8_t notes[]) {
    //notes[note] = velocity;
    notes[note & 0x7F] = 127;
}

void noteOff(uint8_t note, uint8_t velocity, uint8_t notes[]) {
    notes[note & 0x7F] = 0;
}

void clearNotes(uint8_t notes[]) {
    for (int i=0; i < 128; i++) { notes[i] = 0; }
}


// reads and parses raw midi bytes from stream, then updates notes[]
void readMidi(FILE *stream, uint8_t notes[]) {
    
    uint8_t cmd_byte=0, data1_byte, data2_byte;

    // while stream not empty, read a byte at a time
    // (only works if enter key pressed???)

    while( fread( &cmd_byte, 1, 1, stream ) ) {
        
        //cmd_byte = 0x80; // TEST
        fprintf(stderr, "%X ", cmd_byte);  // DEBUG TEST

        switch (cmd_byte & 0xF0) {

        case 0x80: // Note Off
            data1_byte = 60;  // note
            data2_byte = 64;  // velocity
            noteOff(data1_byte, data2_byte, notes);
            break;

        case 0x90: // Note On
            data1_byte = 60;  // note
            data2_byte = 64;  // velocity
            noteOn(data1_byte, data2_byte, notes);
            break;
        }

        if (cmd_byte == 0xFF) {
            // System Reset
            clearNotes(notes);
        }
    }
}


// --------------------------------------------------------------------------------
// TESTS

void test1(uint8_t notes[]) {

    static uint8_t old_note=0, new_note=60;
    static int dir=1;

    noteOff(old_note, 0, notes);
    noteOn(new_note, 127, notes);

    old_note = new_note;
    new_note += dir;
    if (new_note > 80) { dir = -1; }
    if (new_note < 40) { dir = +1; }
}

void test2(uint8_t notes[]) {

    static uint8_t old_note=0;
    uint8_t new_note = randomInt(50, 70);
    noteOff(old_note, 0, notes);
    noteOn(new_note, 127, notes);
    old_note = new_note;
}

// --------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    srandom(time(NULL)); // seed the random generator

    int width = DISPLAY_WIDTH;
    int height = DISPLAY_HEIGHT;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, width, height);
    canvas.Clear();

    // init vars    
    uint8_t notes[128];
    clearNotes(notes);
    int count = 0;

    while (1) {

        if (count % 3 == 0) { test2(notes); }  // TEST

        //readMidi(stdin, notes);

        scrollUp(canvas);
        drawNotes(notes, canvas);

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();
        usleep(DELAY * 1000);

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
