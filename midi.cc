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

// ------------------------------------------------
// square 8x5 thing we have:
// 8 horizontal buttons correspond to channel 0..7
// 5 vertical buttons correspond to note 0x35..0x39
// ------------------------------------------------

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
#include <sys/select.h>

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
        color = Color(0, v, 0);  // TODO (from channel?)
        canvas.SetPixel(x, height-1, color);
        i++;
    }
}

// note = 0 to 127
void noteOn(uint8_t note, uint8_t velocity, uint8_t channel, uint8_t notes[]) {
    //notes[note] = velocity;
    fprintf(stderr, "ON : ch=%x note=0x%02X velocity=0x%02X\n",
            channel, note, velocity);
    notes[note & 0x7F] = 127;  // TODO: encode channel
}

void noteOff(uint8_t note, uint8_t velocity, uint8_t channel, uint8_t notes[]) {
    fprintf(stderr, "OFF: ch=%x note=0x%02X velocity=0x%02X\n",
            channel, note, velocity);
    notes[note & 0x7F] = 0;  // TODO: encode channel.
}

void clearNotes(uint8_t notes[]) {
    for (int i=0; i < 128; i++) { notes[i] = 0; }
}


// reads and parses raw midi bytes from stream, then updates notes[]
void readMidi(int fd, uint8_t notes[], int timeout_ms) {
    
    uint8_t cmd_byte=0, data1_byte, data2_byte;

    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * timeout_ms;

    for (;;) {
        FD_SET(fd, &rfds);
        // Here, we are using the linux feature to update the remaining
        // time. Not really portable, we should really adapt the remaining
        // timeout to whatever we have.
        const int sel = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (sel == 0)
            return;  // time-slice is up.

        if (sel < 0) {
            perror("Error on filedescriptor");
            exit(0);
        }

        if (read(fd, &cmd_byte, 1) != 1)
            continue;

        const uint8_t channel = cmd_byte & 0x0f;
        
        switch (cmd_byte & 0xF0) {
        case 0x80: // Note Off
            if (read(fd, &data1_byte, 1) == 0) return;  // note
            if (read(fd, &data2_byte, 1) == 0) return;  // velocity
            noteOff(data1_byte, data2_byte, channel, notes);
            break;

        case 0x90: // Note On
            if (read(fd, &data1_byte, 1) == 0) return;  // note
            if (read(fd, &data2_byte, 1) == 0) return;  // velocity
            noteOn(data1_byte, data2_byte, channel, notes);
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

    noteOff(old_note, 0, 1, notes);
    noteOn(new_note, 127, 1, notes);

    old_note = new_note;
    new_note += dir;
    if (new_note > 80) { dir = -1; }
    if (new_note < 40) { dir = +1; }
}

void test2(uint8_t notes[]) {

    static uint8_t old_note=0;
    uint8_t new_note = randomInt(50, 70);
    noteOff(old_note, 0, 1, notes);
    noteOn(new_note, 127, 1, notes);
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

        //if (count % 3 == 0) { test2(notes); }  // TEST

        readMidi(STDIN_FILENO, notes, DELAY);

        scrollUp(canvas);
        drawNotes(notes, canvas);

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
