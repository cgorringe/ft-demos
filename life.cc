// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// life
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 7/10/2016
//
// Displays Conway's Game of Life.
//
// How to run:
//
// To see command line options:
//  ./life -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./life -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./life
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
#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <string>
#include <string.h>
#include <signal.h>

// Defaults
#define Z_LAYER 2      // (0-15) 0=background
#define DELAY 200
#define NUM_DOTS 6

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
double opt_respawn = 0;
int opt_width  = DISPLAY_WIDTH;
int opt_height = DISPLAY_HEIGHT;
int opt_xoff=0, opt_yoff=0;
int opt_delay  = DELAY;
bool opt_fgcolor = false, opt_bgcolor = false;
int opt_fg_R=0, opt_fg_G=0, opt_fg_B=0;
int opt_bg_R=0, opt_bg_G=0, opt_bg_B=0;
int opt_num_dots = NUM_DOTS;

int usage(const char *progname) {

    fprintf(stderr, "Game of Life (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 2)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-r <seconds>   : Respawn random dots after given seconds.\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 200)\n"
        "\t-c <RRGGBB>    : Forground color in hex (-c0 = transparent, default cycles)\n"
        "\t-b <RRGGBB>    : Background color in hex (-b0 = #010101, default transparent)\n"
        "\t-n <number>    : Initialize with 1/n random dots. (default 6)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:r:h:d:c:b:n:")) != -1) {
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
        case 'r':  // respawn
            if (sscanf(optarg, "%lf", &opt_respawn) != 1 || opt_respawn < 0) {
                fprintf(stderr, "Invalid respawn '%s'\n", optarg);
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
        case 'c':  // forground color
            if (sscanf(optarg, "%02x%02x%02x", &opt_fg_R, &opt_fg_G, &opt_fg_B) != 3) {
                opt_fg_R=0, opt_fg_G=0, opt_fg_B=0;
                //fprintf(stderr, "Color parse error\n");
                //return usage(argv[0]);
            }
            opt_fgcolor = true;
            break;
        case 'b':  // background color
            if (sscanf(optarg, "%02x%02x%02x", &opt_bg_R, &opt_bg_G, &opt_bg_B) != 3) {
                opt_bg_R=1, opt_bg_G=1, opt_bg_B=1;
            }
            opt_bgcolor = true;
            break;
        case 'n':  // init with 1/n number of dots
            if (sscanf(optarg, "%d", &opt_num_dots) != 1 || opt_num_dots < 2) {
                fprintf(stderr, "Invalid number of dots '%s'\n", optarg);
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

void colorGradient(int start, int end, int r1, int g1, int b1, int r2, int g2, int b2, Color palette[]) {
    float k;
    for (int i=0; i <= (end - start); i++) {
        k = (float)i / (float)(end - start);
        palette[start + i].r = (uint8_t)(r1 + (r2 - r1) * k);
        palette[start + i].g = (uint8_t)(g1 + (g2 - g1) * k);
        palette[start + i].b = (uint8_t)(b1 + (b2 - b1) * k);
    }
}

void initGameOfLife(int width, int height, uint8_t pixels[]) {

    for (int i=0; i < width * height; i++) { 
        pixels[i] = randomInt(0, opt_num_dots - 1) ? 0 : 1;
    }

}

void runGameOfLife(int width, int height, uint8_t pixels[]) {
    
    uint8_t pix2[width * height];
    for (int y=0; y < height; y++) {
        for (int x=0; x < width; x++) {
            int n = 0;
            for (int y1 = y - 1; y1 <= y + 1; y1++) {
                for (int x1 = x - 1; x1 <= x + 1; x1++) {
                    if (pixels[ (((y1 + height) % height) * width) + ((x1 + width) % width) ]) {
                        n++;
                    }
                }
            }
            if (pixels[y * width + x]) { n--; }
            pix2[y * width + x] = (n == 3 || (n == 2 && pixels[y * width + x] ));
        }
    }

    for (int i=0; i < width * height; i++) {
        pixels[i] = pix2[i];
    }    
}

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // seed the random generator
    srandom(time(NULL));

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

    // setup colors
    Color fg_color, bg_color = Color(opt_bg_R, opt_bg_G, opt_bg_B);
    if (opt_fgcolor) {
        fg_color = Color(opt_fg_R, opt_fg_G, opt_fg_B);
    }

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // pixel buffer
    uint8_t pixels[ opt_width * opt_height ];
    for (int i=0; i < opt_width * opt_height; i++) { pixels[i] = 0; }  // clear pixel buffer

    initGameOfLife(opt_width, opt_height, pixels);

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // other vars
    int count = 0, colr = 0;
    time_t starttime = time(NULL);
    time_t respawn_time = starttime;

    do {
        runGameOfLife(opt_width, opt_height, pixels);

        // check for respawn
        if (opt_respawn > 0) {
            if (difftime(time(NULL), respawn_time) > opt_respawn) {
                respawn_time = time(NULL);
                initGameOfLife(opt_width, opt_height, pixels);
            }
        }

        // set pixel color if cycling through palette
        if (!opt_fgcolor) {
            fg_color = palette[colr];
        }

        // copy pixel buffer to canvas
        int dst = 0;
        for (int y=0; y < opt_height; y++) {
            for (int x=0; x < opt_width; x++) {
                canvas.SetPixel( x, y, (pixels[dst] ? fg_color : bg_color) );
                dst++;
            }
        }

        // send canvas
        canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
        canvas.Send();
        usleep(opt_delay * 1000);

        count++;
        if (count == INT_MAX) { count=0; }

        colr++;
        if (colr >= 256) { colr=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received );

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    if (interrupt_received) return 1;
    return 0;
}
