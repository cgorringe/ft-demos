// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// plasma2
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Displays animated plasma effect on the Flaschen Taschen.
// This version uses anti-aliasing to smooth out jittering by 
// supersampling by 2x and down-sampling to the display resolution.
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./plasma2 localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./plasma2
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
#define Z_LAYER 8      // (0-15) 0=background
#define DELAY 10
#define PALETTE_MAX 4  // 0=Rainbow, 1=Nebula, 2=Fire, 3=Bluegreen, 4=RGB

void colorGradient(int start, int end, int r1, int g1, int b1, int r2, int g2, int b2, Color palette[]) {
    float k;
    for (int i=0; i <= (end - start); i++) {
        k = (float)i / (float)(end - start);
        palette[start + i].r = (uint8_t)(r1 + (r2 - r1) * k);
        palette[start + i].g = (uint8_t)(g1 + (g2 - g1) * k);
        palette[start + i].b = (uint8_t)(b1 + (b2 - b1) * k);
    }
}

void setPalette(int num, Color palette[]) {
    switch (num) {
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
        // Nebula
        colorGradient(   0,  31,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  32,  95,   0,   0, 127, 127,   0, 255, palette );  // half blue -> blue-violet
        colorGradient(  96, 159, 127,   0, 255, 255,   0,   0, palette );  // blue-violet -> red
        colorGradient( 160, 191, 255,   0,   0, 255, 255, 255, palette );  // red -> white
        colorGradient( 192, 255, 255, 255, 255,   1,   1,   1, palette );  // white -> black
        break;
      case 2:
        // Fire
        colorGradient(   0,  23,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  24,  47,   0,   0, 127, 255,   0,   0, palette );  // half blue -> red
        colorGradient(  48,  95, 255,   0,   0, 255, 255,   0, palette );  // red -> yellow
        colorGradient(  96, 127, 255, 255,   0, 255, 255, 255, palette );  // yellow -> white
        colorGradient( 128, 159, 255, 255, 255, 255, 255,   0, palette );  // white -> yellow
        colorGradient( 160, 207, 255, 255,   0, 255,   0,   0, palette );  // yellow -> red
        colorGradient( 208, 231, 255,   0,   0,   0,   0, 127, palette );  // red -> half blue
        colorGradient( 232, 255,   0,   0, 127,   1,   1,   1, palette );  // half blue -> black
        break;
      case 3:
        // Bluegreen
        colorGradient(   0,  23,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  24,  47,   0,   0, 127,   0, 127, 255, palette );  // half blue -> teal
        colorGradient(  48,  95,   0, 127, 255,   0, 255,   0, palette );  // teal -> green
        colorGradient(  96, 127,   0, 255,   0, 255, 255, 255, palette );  // green -> white
        colorGradient( 128, 159, 255, 255, 255,   0, 255,   0, palette );  // white -> green
        colorGradient( 160, 207,   0, 255,   0,   0, 127, 255, palette );  // green -> teal
        colorGradient( 208, 231,   0, 127, 255,   0,   0, 127, palette );  // teal -> half blue
        colorGradient( 232, 255,   0,   0, 127,   1,   1,   1, palette );  // half blue -> black
        break;
      case 4:
        // RGB + White
        colorGradient(   0,  63,   1,   1,   1, 255,   0,   0, palette );  // black -> red
        colorGradient(  64, 127,   1,   1,   1,   0, 255,   0, palette );  // black -> green
        colorGradient( 128, 191,   1,   1,   1,   0,   0, 255, palette );  // black -> blue
        colorGradient( 192, 255,   1,   1,   1, 255, 255, 255, palette );  // black -> white
        break;
    }
}

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // will use default if not set otherwise
    if (argc > 1) {
        hostname = argv[1];        // hostname can be supplied as first arg
    }

    int scale = 4;
    int width = DISPLAY_WIDTH;
    int height = DISPLAY_HEIGHT;
    int dwidth = width * scale;
    int dheight = height * scale;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, width, height);
    canvas.Clear();

    // set the color palette
    Color palette[256];
    //setPalette(0);

    // pixel buffer
    uint8_t pixels[ dwidth * dheight ];

    // init precalculated plasma buffers
    uint8_t plasma1[ dwidth * dheight * 4 ];
    uint8_t plasma2[ dwidth * dheight * 4 ];
    int dst = 0;
    for (int y=0; y < (dheight * 2); y++) {
        for (int x=0; x < (dwidth * 2); x++) {
            // ** TODO: redo consts?? **
            plasma1[dst] = (uint8_t)(64 + 63 * sin( sqrt( (double)((dheight-y)*(dheight-y)) 
                                                          + ((dwidth-x)*(dwidth-x)) ) / 16 ));  // 5*scale
//            plasma2[dst] = (uint8_t)(64 + 63 * sin( (double) x / (12 + 4.5 * cos((double) y / (19 * scale))) )
//                                             * cos( (double) y / (10 + 3.5 * sin((double) x / (14 * scale))) ) );
            plasma2[dst] = (uint8_t)(64 + 63 * sin( (double) x / (37 + 15 * cos((double) y / 74)) )
                                             * cos( (double) y / (31 + 11 * sin((double) x / 57)) ) );
            dst++;
        }
    }

    //double foo = 3;   // for 1x
    //double foo = 10;  // for 2x
    double foo = 10;    // for 4x
    int x1, y1, x2, y2, x3, y3, src1, src2, src3;
    int hw = (dwidth >> 1);
    int hh = (dheight >> 1);
    int count = 0;
    int curPalette = 0;

    while (1) {

        // set new color palette
        if ((count % 2000) == 0) {
            setPalette(curPalette, palette);
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 0; }
        }

        // move plasma with sine functions
        x1 = hw  + (int)(hw * cos( (double)  count /  97 / foo ));
        x2 = hw  + (int)(hw * sin( (double) -count / 114 / foo ));
        x3 = hw  + (int)(hw * sin( (double) -count / 137 / foo ));
        y1 = hh + (int)(hh * sin( (double)  count / 123 / foo ));
        y2 = hh + (int)(hh * cos( (double) -count /  75 / foo ));
        y3 = hh + (int)(hh * cos( (double) -count / 108 / foo ));
        src1 = y1 * dwidth * 2 + x1;
        src2 = y2 * dwidth * 2 + x2;
        src3 = y3 * dwidth * 2 + x3;

        // write plasma to pixel buffer
        dst = 0;
        for (int y=0; y < dheight; y++) {
            for (int x=0; x < dwidth; x++) {
                // plot pixel as sum of plasma functions
                pixels[dst] = (uint8_t)((plasma1[src1] + plasma2[src2] + plasma2[src3]) & 0xFF);
                //pixels[dst] = (uint8_t)((plasma1[src1]) & 0xFF);
                //pixels[dst] = (uint8_t)((plasma2[src2]) & 0xFF);
                dst++; src1++; src2++; src3++;
            }
            // skip to next line in plasma buffers
            src1 += dwidth; src2 += dwidth; src3 += dwidth;
        }

        // copy pixel buffer to canvas
        uint8_t dot_r, dot_g, dot_b;
        int src = 0;
        dst = 0;
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                
                // anti-alias by down-sampling (averaging) 4 pixels to 1

                // TEST
                /*
                dot_r = palette[pixels[src]].r;
                dot_g = palette[pixels[src]].g;
                dot_b = palette[pixels[src]].b;
                //*/

              if (scale == 2) {
                // subsample 2x2 pixels to 1
                dot_r = ( palette[pixels[src]].r + palette[pixels[src + 1]].r 
                        + palette[pixels[src + dwidth]].r + palette[pixels[src + dwidth + 1]].r ) >> 2;
                dot_g = ( palette[pixels[src]].g + palette[pixels[src + 1]].g 
                        + palette[pixels[src + dwidth]].g + palette[pixels[src + dwidth + 1]].g ) >> 2;
                dot_b = ( palette[pixels[src]].b + palette[pixels[src + 1]].b 
                        + palette[pixels[src + dwidth]].b + palette[pixels[src + dwidth + 1]].b ) >> 2;
              }
              else if (scale == 4) {
                // subsample 4x4 pixels to 1
                dot_r = ( palette[pixels[src + 0]].r + palette[pixels[src + 1]].r
                        + palette[pixels[src + 2]].r + palette[pixels[src + 3]].r
                        + palette[pixels[src + dwidth + 0]].r + palette[pixels[src + dwidth + 1]].r
                        + palette[pixels[src + dwidth + 2]].r + palette[pixels[src + dwidth + 3]].r
                        + palette[pixels[src + (dwidth * 2) + 0]].r + palette[pixels[src + (dwidth * 2) + 1]].r
                        + palette[pixels[src + (dwidth * 2) + 2]].r + palette[pixels[src + (dwidth * 2) + 3]].r
                        + palette[pixels[src + (dwidth * 3) + 0]].r + palette[pixels[src + (dwidth * 3) + 1]].r
                        + palette[pixels[src + (dwidth * 3) + 2]].r + palette[pixels[src + (dwidth * 3) + 3]].r
                        ) >> 4;
                dot_g = ( palette[pixels[src + 0]].g + palette[pixels[src + 1]].g
                        + palette[pixels[src + 2]].g + palette[pixels[src + 3]].g
                        + palette[pixels[src + dwidth + 0]].g + palette[pixels[src + dwidth + 1]].g
                        + palette[pixels[src + dwidth + 2]].g + palette[pixels[src + dwidth + 3]].g
                        + palette[pixels[src + (dwidth * 2) + 0]].g + palette[pixels[src + (dwidth * 2) + 1]].g
                        + palette[pixels[src + (dwidth * 2) + 2]].g + palette[pixels[src + (dwidth * 2) + 3]].g
                        + palette[pixels[src + (dwidth * 3) + 0]].g + palette[pixels[src + (dwidth * 3) + 1]].g
                        + palette[pixels[src + (dwidth * 3) + 2]].g + palette[pixels[src + (dwidth * 3) + 3]].g
                        ) >> 4;
                dot_b = ( palette[pixels[src + 0]].b + palette[pixels[src + 1]].b
                        + palette[pixels[src + 2]].b + palette[pixels[src + 3]].b
                        + palette[pixels[src + dwidth + 0]].b + palette[pixels[src + dwidth + 1]].b
                        + palette[pixels[src + dwidth + 2]].b + palette[pixels[src + dwidth + 3]].b
                        + palette[pixels[src + (dwidth * 2) + 0]].b + palette[pixels[src + (dwidth * 2) + 1]].b
                        + palette[pixels[src + (dwidth * 2) + 2]].b + palette[pixels[src + (dwidth * 2) + 3]].b
                        + palette[pixels[src + (dwidth * 3) + 0]].b + palette[pixels[src + (dwidth * 3) + 1]].b
                        + palette[pixels[src + (dwidth * 3) + 2]].b + palette[pixels[src + (dwidth * 3) + 3]].b
                        ) >> 4;
              }

                //canvas.SetPixel( x, y, palette[ pixels[dst] ] );
                canvas.SetPixel( x, y, Color(dot_r, dot_g, dot_b) );
                dst++; src += scale;
            }
            //src += dwidth;  // skip every other row (2x)
            src += (dwidth * 2);  // skip every other row (4x)
        }

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();
        usleep(DELAY * 1000);

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
