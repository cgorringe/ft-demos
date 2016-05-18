// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// black
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Clears the Flaschen Taschen canvas.
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./black -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./black
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

#define DISPLAY_WIDTH  (9*5)  // large: 9*5
#define DISPLAY_HEIGHT (7*5)  //        7*5
#define Z_LAYER 0      // (0-15) 0=background

// ------------------------------------------------------------------------------------------
// Command Line Options

// option vars
const char *opt_hostname = NULL;
int opt_layer  = Z_LAYER;
double opt_timeout = 0;  // timeout now
int opt_width  = DISPLAY_WIDTH;
int opt_height = DISPLAY_HEIGHT;
bool opt_black = false;
bool opt_all = false;
bool opt_fill = false;
int opt_r=0, opt_g=0, opt_b=0;

int usage(const char *progname) {

    fprintf(stderr, "Black (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options] [all]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-l <layer>     : Layer 0-15. (default 0)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default now)\n"
        "\t-g <W>x<H>     : Output geometry. (default 45x35)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-b             : Black out with color (1,1,1)\n"
        "\t-c <RRGGBB>    : Fill with color as hex\n"
        "\t all           : Clear ALL layers\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "l:t:g:h:bc:")) != -1) {
        switch (opt) {
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
        case 'g':  // geometry
            if (sscanf(optarg, "%dx%d", &opt_width, &opt_height) < 2) {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':  // hostname
            opt_hostname = strdup(optarg); // leaking. Ignore.
            break;
        case 'b':  // black out
            opt_black = true;
            break;
        case 'c':
            if (sscanf(optarg, "%02x%02x%02x", &opt_r, &opt_g, &opt_b) != 3) {
                fprintf(stderr, "Color parse error\n");
                return usage(argv[0]);
            }
            opt_fill = true;
            break;
        default:
            usage(argv[0]);
        }
    }

    // retrieve arg text
    const char *text = argv[optind];
    if (text && strncmp(text, "all", 3) == 0) {
        opt_all = true;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);

    // color, black, or clear
    if (opt_fill) {
        canvas.Fill(Color(opt_r, opt_g, opt_b));
    }
    else if (opt_black) {
        canvas.Fill(Color(1, 1, 1));
    }
    else {
        canvas.Clear();
    }

    if (opt_all) {
        printf("clear all layers\n");
    }
    else {
        printf("clear layer %d\n", opt_layer);
    }

    time_t starttime = time(NULL);
    do {
        if (opt_all) {
            // clear ALL layers
            for (int i=0; i <= 15; i++) {
                canvas.SetOffset(0, 0, i);
                canvas.Send();
            }
        }
        else {
            // clear single layer
            canvas.SetOffset(0, 0, opt_layer);
            canvas.Send();
        }

        sleep(1);

    } while ( difftime(time(NULL), starttime) <= opt_timeout );

    return 0;
}
