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
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./quilt localhost
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//                               large  small
#define DISPLAY_WIDTH  (5*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (4*5)  //  7*5    4*5
#define ZLAYER 3              // 0 for background layer
#define SKIP_NUM 5            // width or height of crate

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // const Color red(255, 0, 0);
    // canvas.SetPixel(0, 0, red);

    canvas.Clear();

    while (1) {
        int r = arc4random_uniform(256);
        int g = arc4random_uniform(256);
        int b = arc4random_uniform(256);

        int x1 = arc4random_uniform(SKIP_NUM);
        int y1 = arc4random_uniform(SKIP_NUM);

        for (int y=y1; y < DISPLAY_HEIGHT; y += SKIP_NUM) {
            for (int x=x1; x < DISPLAY_WIDTH; x += SKIP_NUM) {

                canvas.SetPixel(x, y, Color(r, g, b));
                canvas.SetPixel(DISPLAY_WIDTH - x, y, Color(r, g, b));
                canvas.SetPixel(x, DISPLAY_HEIGHT - y, Color(r, g, b));
                canvas.SetPixel(DISPLAY_WIDTH - x, DISPLAY_HEIGHT - y, Color(r, g, b));

                // TEST: width and height may not be equal
                canvas.SetPixel(y, x, Color(r, g, b));
                canvas.SetPixel(DISPLAY_WIDTH - y, x, Color(r, g, b));
                canvas.SetPixel(y, DISPLAY_HEIGHT - x, Color(r, g, b));
                canvas.SetPixel(DISPLAY_WIDTH - y, DISPLAY_HEIGHT - x, Color(r, g, b));
            }
        }

        // send canvas
        canvas.SetOffset(0, 0, ZLAYER);
        canvas.Send();
        usleep(200 * 1000);
    }
}
