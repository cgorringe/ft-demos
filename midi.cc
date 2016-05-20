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
// To see command line options:
//  ./midi -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
// cat /dev/midi | ./midi -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
// export FT_DISPLAY=localhost
// cat /dev/midi | ./midi
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

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <sys/select.h>
#include <string.h>
#include <string>
#include <signal.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 14       // (0-15) 0=background
#define DELAY 50

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

// ------------------------------------------------------------------------------------------
// Command Line Options

// option vars
const char *opt_hostname = NULL;
int opt_layer  = Z_LAYER;
double opt_timeout = 60*60*24;  // timeout in 24 hrs
int opt_width  = DISPLAY_WIDTH;
int opt_height = DISPLAY_HEIGHT;
int opt_xoff=0, opt_yoff=0;
int opt_delay  = DELAY;
int opt_r=0x00, opt_g=0xFF, opt_b=0x00;

int usage(const char *progname) {

    fprintf(stderr, "midi (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: cat /dev/midi | %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 50)\n"
        "\t-c <RRGGBB>    : Note color as hex (default green)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:c:")) != -1) {
        switch (opt) {
        case '?':  // help
            return usage(argv[0]);
            break;
        case 'g':  // geometry
            if (sscanf(optarg, "%dx%d%d%d", &opt_width, &opt_height, &opt_xoff, &opt_yoff) < 2) {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'l':  // layer
            if (sscanf(optarg, "%d", &opt_layer) != 1 || opt_layer < 0 || opt_layer >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 't':  // timeout
            if (sscanf(optarg, "%lf", &opt_timeout) != 1 || opt_timeout < 0) {
                fprintf(stderr, "Invalid timeout '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':  // hostname
            opt_hostname = strdup(optarg); // leaking. Ignore.
            break;
        case 'd':  // delay
            if (sscanf(optarg, "%d", &opt_delay) != 1 || opt_delay < 1) {
                fprintf(stderr, "Invalid delay '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'c':
            if (sscanf(optarg, "%02x%02x%02x", &opt_r, &opt_g, &opt_b) != 3) {
                fprintf(stderr, "Color parse error\n");
                return usage(argv[0]);
            }
            break;
        default:
            return usage(argv[0]);
        }
    }
    return 0;
}

// ------------------------------------------------------------------------------------------

// random int in range min to max inclusive
int randomInt(int min, int max) {
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
    Color color(opt_r, opt_g, opt_b);
    //int v;

    if (width > 120) { return; } // never happens
    int i = 60 - (width >> 1);  // center on middle-C (60)

    for (int x=0; x < width; x++) {
        // v = notes[i] * 2;
        // color = Color(0, v, 0);  // not used
        canvas.SetPixel(x, height-1, color);
        i++;
    }
}

// note = 0 to 127
void noteOn(uint8_t note, uint8_t velocity, uint8_t channel, uint8_t notes[]) {
    //notes[note] = velocity;
    //fprintf(stderr, "ON : ch=%x note=0x%02X velocity=0x%02X\n",
    //        channel, note, velocity);
    notes[note & 0x7F] = 127;  // TODO: encode channel
}

void noteOff(uint8_t note, uint8_t velocity, uint8_t channel, uint8_t notes[]) {
    //fprintf(stderr, "OFF: ch=%x note=0x%02X velocity=0x%02X\n",
    //        channel, note, velocity);
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

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    srandom(time(NULL)); // seed the random generator

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // init vars    
    uint8_t notes[128];
    clearNotes(notes);
    int count = 0;

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    time_t starttime = time(NULL);

    do {
        /*
        // TEST
        if (count % 3 == 0) { 
            test2(notes); 
            usleep(DELAY * 1000);
        }
        //*/

        readMidi(STDIN_FILENO, notes, opt_delay);

        scrollUp(canvas);
        drawNotes(notes, canvas);

        // send canvas
        canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
        canvas.Send();

        count++;
        if (count == INT_MAX) { count=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received );

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    if (interrupt_received) return 1;
    return 0;
}
