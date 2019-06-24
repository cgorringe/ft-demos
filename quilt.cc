// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// quilt
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// 4/22/2016
//
// Displays animated quilt pattern on the Flaschen Taschen.
// https://noisebridge.net/wiki/Flaschen_Taschen
//
// How to run:
//
// To see command line options:
//  ./quilt -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./quilt -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./quilt
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
#include <time.h>
#include <string>
#include <string.h>
#include <signal.h>

#define Z_LAYER 2      // (0-15) 0=background
#define DELAY 10  // 200
#define SKIP_NUM 5            // width or height of crate


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
int opt_delay   = DELAY;

int usage(const char *progname) {

    fprintf(stderr, "Quilt (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 10)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:")) != -1) {
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

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    srandom(time(NULL)); // seed the random generator

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);

    canvas.Clear();

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    time_t starttime = time(NULL);
    bool quit = false;
    int w = opt_width, h = opt_height;
    //int w = opt_width - 1, h = opt_height - 1;

    do {
        int r = randomInt(0, 255);
        int g = randomInt(0, 255);
        int b = randomInt(0, 255);

        int x1 = randomInt(0, SKIP_NUM - 1);
        int y1 = randomInt(0, SKIP_NUM - 1);

        for (int y=y1; y < opt_height; y += SKIP_NUM) {
            for (int x=x1; x < opt_width; x += SKIP_NUM) {

                canvas.SetPixel(x, y, Color(r, g, b));
                canvas.SetPixel(w - x, y, Color(r, g, b));
                canvas.SetPixel(x, h - y, Color(r, g, b));
                canvas.SetPixel(w - x, h - y, Color(r, g, b));

                // width and height may not be equal but that's OK
                canvas.SetPixel(y, x, Color(r, g, b));
                canvas.SetPixel(w - y, x, Color(r, g, b));
                canvas.SetPixel(y, h - x, Color(r, g, b));
                canvas.SetPixel(w - y, h - x, Color(r, g, b));

                // send canvas
                canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
                canvas.Send();

                if ( (difftime(time(NULL), starttime) >= opt_timeout) || interrupt_received ) {
                    quit = true;
                    break;
                }
                usleep(opt_delay * 1000);
            }
            if (quit) break;
        }
    } while (!quit);

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    if (interrupt_received) return 1;
    return 0;
}
