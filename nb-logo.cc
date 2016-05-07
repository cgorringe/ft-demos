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
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./nb-logo ht.noise
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//                               large  small
#define DISPLAY_WIDTH  (5*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (4*5)  //  7*5    4*5
#define Z_LAYER 9      // (0-15) 0=background
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
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

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

    // Open socket and create our frame.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen *frame = new UDPFlaschenTaschen(socket, LOGO_WIDTH + 2, LOGO_HEIGHT + 2);

    int colr = 0;
    int x=-1, y=-1, sx=1, sy=1;

    while (1) {

        // draw the logo
        updateFromPattern(frame, nb_logo, palette[colr]);
        frame->SetOffset(x, y, Z_LAYER);
        frame->Send();
        usleep(DELAY * 1000);

        // animate the logo
        if ((colr % 8) == 0) {
            x += sx;
            if (x > (DISPLAY_WIDTH - LOGO_WIDTH)) {
                x -= sx; sy = 1; y += sy;
            }
            if (y > (DISPLAY_HEIGHT - LOGO_HEIGHT)) {
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
    }
}
