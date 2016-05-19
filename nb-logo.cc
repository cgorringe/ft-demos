// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// nb-logo
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// 4/29/2016
//
// Displays an animated Noisebridge Logo on the Flaschen Taschen.
// https://noisebridge.net/wiki/Flaschen_Taschen
//
// How to run:
//
// To see command line options:
//  ./nb-logo -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./nb-logo -h ht.noise
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./nb-logo
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

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string>

// Defaults                      large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 8      // (0-15) 0=background
#define DELAY 40  // 20

#define LOGO_WIDTH 16
#define LOGO_HEIGHT 15

static const char* nb_logo[LOGO_HEIGHT] = {
    "      ##.       ",
    "     #..#.      ",
    "   ###  ###.    ",
    "  #...  ...#.   ",
    "  #.      .#. #.",
    "##.      ..#.##.",
    "..###.   ####.#.",
    "###..    #..#.#.",
    "..###.   #..#.#.",
    "###..    ####.#.",
    "...## .. ..#.##.",
    "  #...##.  #..#.",
    "  #..#..#..#. . ",
    "   ###. ###.    ",
    "   ...  ...     "
};

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
bool opt_color = false;
int opt_r=0, opt_g=0, opt_b=0;

int usage(const char *progname) {

    fprintf(stderr, "nb-logo (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 8)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 40)\n"
        "\t-c <RRGGBB>    : Logo color as hex (default cycles)\n"
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
            opt_color = true;
            break;
        default:
            return usage(argv[0]);
        }
    }
    return 0;
}

// ------------------------------------------------------------------------------------------

void colorGradient(int start, int end, int r1, int g1, int b1, int r2, int g2, int b2, Color palette[]) {
    float k;
    for (int i=0; i <= (end - start); i++) {
        k = (float)i / (float)(end - start);
        palette[start + i].r = (uint8_t)(r1 + (r2 - r1) * k);
        palette[start + i].g = (uint8_t)(g1 + (g2 - g1) * k);
        palette[start + i].b = (uint8_t)(b1 + (b2 - b1) * k);
    }
}

void updateFromPattern(UDPFlaschenTaschen *frame, const char *pattern[], const Color &color) {

    frame->Clear();
    Color black = Color(1, 1, 1);
    for (int y=0; y < LOGO_HEIGHT; y++) {
        const char *line = pattern[y];
        for (int x=0; line[x]; x++) {
            if (line[x] == '#') {
                frame->SetPixel(x + 1, y + 1, color);
            }
            else if (line[x] == '.') {
                frame->SetPixel(x + 1, y + 1, black);
            }
        }
    }
}


int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // set the color palette to a rainbow of colors
    Color palette[256];
    colorGradient( 0,   31,  255, 0,   255, 0,   0,   255, palette );
    colorGradient( 32,  63,  0,   0,   255, 0,   255, 255, palette );
    colorGradient( 64,  95,  0,   255, 255, 0,   255,   0, palette );
    colorGradient( 96,  127, 0,   255, 0,   127, 255,   0, palette );
    colorGradient( 128, 159, 127, 255, 0,   255, 255,   0, palette );
    colorGradient( 160, 191, 255, 255, 0,   255, 127,   0, palette );
    colorGradient( 192, 223, 255, 127, 0,   255, 0,     0, palette );
    colorGradient( 224, 255, 255, 0,   0,   255, 0,   255, palette );

    // setup color
    Color logo_color;
    if (opt_color) {
        logo_color = Color(opt_r, opt_g, opt_b);
    }        

    // Open socket and create our frame.
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen *frame = new UDPFlaschenTaschen(socket, LOGO_WIDTH + 2, LOGO_HEIGHT + 2);

    int colr = 0;
    int x=-1, y=-1, sx=1, sy=1;

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    time_t starttime = time(NULL);

    do {
        // draw the logo
        if (opt_color) {
            updateFromPattern(frame, nb_logo, logo_color);
        }
        else {
            updateFromPattern(frame, nb_logo, palette[colr]);
        }

        frame->SetOffset(opt_xoff + x, opt_yoff + y, opt_layer);
        frame->Send();
        usleep(opt_delay * 1000);

        // animate the logo
        if ((colr % 8) == 0) {
            x += sx;
            if (x > (opt_width - LOGO_WIDTH)) {
                x -= sx; sy = 1; y += sy;
            }
            if (y > (opt_height - LOGO_HEIGHT)) {
                y -= sy; sx = -1; x += sx;
            }
            if (x < -1) {
                x -= sx; sy = -1; y += sy;
            }
            if (y < -1) {
                y -= sy; sx = 1; x += sx;
            }
        }

        colr++;
        if (colr >= 256) { colr=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received );

    // clear frame on exit
    frame->Clear();
    frame->Send();

    if (interrupt_received) return 1;
    return 0;
}
