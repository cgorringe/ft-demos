// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// plasma
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// 4/28/2016
//
// Displays animated plasma effect on the Flaschen Taschen.
// https://noisebridge.net/wiki/Flaschen_Taschen
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./plasma localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./plasma
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
#include <math.h>
#include <limits.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define ZLAYER 3   // 0 for background layer
#define PALETTE 0  // 0=Rainbow, 1=RGB, 2=Nebula, 3=Fire, 4=Bluegreen

void colorGradient(int start, int end, int r1, int g1, int b1, int r2, int g2, int b2, Color palette[]) {
    float k;
    for (int i=0; i <= (end - start); i++) {
        k = (float)i / (float)(end - start);
        palette[start + i].r = (uint8_t)(r1 + (r2 - r1) * k);
        palette[start + i].g = (uint8_t)(g1 + (g2 - g1) * k);
        palette[start + i].b = (uint8_t)(b1 + (b2 - b1) * k);
    }
}


int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // will use default if not set otherwise
    if (argc > 1) {
        hostname = argv[1];        // hostname can be supplied as first arg
    }

    int width = DISPLAY_WIDTH;
    int height = DISPLAY_HEIGHT;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, width, height);
    canvas.Clear();

    // set the color palette
    Color palette[256];
    switch (PALETTE) {
      case 0:
        // Rainbow
        colorGradient(   0,  35, 255,   0, 255,   0,   0, 255, palette );  // magenta -> blue
        colorGradient(  36,  71,   0,   0, 255,   0, 255, 255, palette );  // blue -> cyan
        colorGradient(  72, 107,   0, 255, 255,   0, 255,   0, palette );  // cyan -> green
        colorGradient( 108, 143,   0, 255,   0, 255, 255,   0, palette );  // green -> yellow
        colorGradient( 144, 179, 255, 255,   0, 255, 127,   0, palette );  // yellow -> orange
        colorGradient( 180, 215, 255, 127,   0, 255,   0,   0, palette );  // orange -> red
        colorGradient( 216, 255, 255,   0,   0, 255,   0, 255, palette );  // red -> magenta
        break;
      case 1:
        // RGB + White
        colorGradient(   0,  63,   0,   0,   0, 255,   0,   0, palette );  // black -> red
        colorGradient(  64, 127,   0,   0,   0,   0, 255,   0, palette );  // black -> green
        colorGradient( 128, 191,   0,   0,   0,   0,   0, 255, palette );  // black -> blue
        colorGradient( 192, 255,   0,   0,   0, 255, 255, 255, palette );  // black -> white
        break;
      case 2:
        // Nebula
        colorGradient(   0,  31,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  32,  95,   0,   0, 127, 127,   0, 255, palette );  // half blue -> blue-violet
        colorGradient(  96, 159, 127,   0, 255, 255,   0,   0, palette );  // blue-violet -> red
        colorGradient( 160, 191, 255,   0,   0, 255, 255, 255, palette );  // red -> white
        colorGradient( 192, 255, 255, 255, 255,   0,   0,   0, palette );  // white -> black
        break;
      case 3:
        // Fire
        colorGradient(   0,  23,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  24,  47,   0,   0, 127, 255,   0,   0, palette );  // half blue -> red
        colorGradient(  48,  95, 255,   0,   0, 255, 255,   0, palette );  // red -> yellow
        colorGradient(  96, 127, 255, 255,   0, 255, 255, 255, palette );  // yellow -> white
        colorGradient( 128, 159, 255, 255, 255, 255, 255,   0, palette );  // white -> yellow
        colorGradient( 160, 207, 255, 255,   0, 255,   0,   0, palette );  // yellow -> red
        colorGradient( 208, 231, 255,   0,   0,   0,   0, 127, palette );  // red -> half blue
        colorGradient( 232, 255,   0,   0, 127,   0,   0,   0, palette );  // half blue -> black
        break;
      case 4:
        // Bluegreen
        colorGradient(   0,  23,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  24,  47,   0,   0, 127,   0, 127, 255, palette );  // half blue -> teal
        colorGradient(  48,  95,   0, 127, 255,   0, 255,   0, palette );  // teal -> green
        colorGradient(  96, 127,   0, 255,   0, 255, 255, 255, palette );  // green -> white
        colorGradient( 128, 159, 255, 255, 255,   0, 255,   0, palette );  // white -> green
        colorGradient( 160, 207,   0, 255,   0,   0, 127, 255, palette );  // green -> teal
        colorGradient( 208, 231,   0, 127, 255,   0,   0, 127, palette );  // teal -> half blue
        colorGradient( 232, 255,   0,   0, 127,   0,   0,   0, palette );  // half blue -> black
        break;
    }

    // pixel buffer
    uint8_t pixels[ width * height ];

    // init precalculated plasma buffers
    uint8_t plasma1[ width * height * 4 ];
    uint8_t plasma2[ width * height * 4 ];
    int dst = 0;
    for (int y=0; y < (height * 2); y++) {
        for (int x=0; x < (width * 2); x++) {
            plasma1[dst] = (uint8_t)(64 + 63 * sin( sqrt( (double)((height-y)*(height-y)) 
                                                          + ((width-x)*(width-x)) ) / 5 ));
            plasma2[dst] = (uint8_t)(64 + 63 * sin( (double) x / (12 + 4.5 * cos((double) y / 19)) )
                                             * cos( (double) y / (10 + 3.5 * sin((double) x / 14)) ) );
            dst++;
        }
    }

    int x1, y1, x2, y2, x3, y3, src1, src2, src3;
    float w, h;
    int halfwidth = (width >> 1);
    int halfheight = (height >> 1);
    int count = 0;
    double foo = 3;

    while (1) {

        // move plasma with sine functions
        w = (float)halfwidth - 1;
        h = (float)halfheight - 1;
        x1 = halfwidth  + (int)(w * cos( (double)  count /  97 / foo ));
        x2 = halfwidth  + (int)(w * sin( (double) -count / 114 / foo ));
        x3 = halfwidth  + (int)(w * sin( (double) -count / 137 / foo ));
        y1 = halfheight + (int)(h * sin( (double)  count / 123 / foo ));
        y2 = halfheight + (int)(h * cos( (double) -count /  75 / foo ));
        y3 = halfheight + (int)(h * cos( (double) -count / 108 / foo ));
        src1 = y1 * width * 2 + x1;
        src2 = y2 * width * 2 + x2;
        src3 = y3 * width * 2 + x3;

        // write plasma to pixel buffer
        dst = 0;
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                // plot pixel as sum of plasma functions
                pixels[dst] = (uint8_t)((plasma1[src1] + plasma2[src2] + plasma2[src3]) & 0xFF);
                dst++; src1++; src2++; src3++;
            }
            // skip to next line in plasma buffers
            src1 += width; src2 += width; src3 += width;
        }

        // copy pixel buffer to canvas
        dst = 0;
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                canvas.SetPixel( x, y, palette[ pixels[dst] ] );
                dst++;
            }
        }

        // send canvas
        canvas.SetOffset(0, 0, ZLAYER);
        canvas.Send();
        usleep(10 * 1000);

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
