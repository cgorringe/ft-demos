// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// blur
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Displays boxes or sound waves with blur effect.
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./blur localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./blur
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
#include <limits.h>
#include <time.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 1      // (0-15) 0=background
#define DELAY 50
#define PALETTE_MAX 2  // 0=Nebula, 1=Fire, 2=Bluegreen
#define DEMO 0         // 0=wave, 1=boxes

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

void setPalette(int num, Color palette[]) {
    switch (num) {
      case 0:
        // Nebula
        colorGradient(   0,  31,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  32,  95,   0,   0, 127, 127,   0, 255, palette );  // half blue -> blue-violet
        colorGradient(  96, 159, 127,   0, 255, 255,   0,   0, palette );  // blue-violet -> red
        colorGradient( 160, 191, 255,   0,   0, 255, 255, 255, palette );  // red -> white
        colorGradient( 192, 255, 255, 255, 255, 255, 255, 255, palette );  // white
        /*
        colorGradient(   0,  63,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127, 127,   0, 255, palette );  // half blue -> blue-violet
        colorGradient( 128, 191, 127,   0, 255, 255,   0,   0, palette );  // blue-violet -> red
        colorGradient( 192, 255, 255,   0,   0, 255, 255, 255, palette );  // red -> white
        //*/
        break;
      case 1:
        // Fire
        colorGradient(   0,  31,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  32,  95,   0,   0, 127, 255,   0,   0, palette );  // half blue -> red
        colorGradient(  96, 159, 255,   0,   0, 255, 255,   0, palette );  // red -> yellow
        colorGradient( 160, 191, 255, 255,   0, 255, 255, 255, palette );  // yellow -> white
        colorGradient( 192, 255, 255, 255, 255, 255, 255, 255, palette );  // white
        /*
        colorGradient(   0,  63,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127, 255,   0,   0, palette );  // half blue -> red
        colorGradient( 128, 191, 255,   0,   0, 255, 255,   0, palette );  // red -> yellow
        colorGradient( 192, 255, 255, 255,   0, 255, 255, 255, palette );  // yellow -> white
        //*/
        break;
      case 2:
        // Bluegreen
        colorGradient(   0,  31,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  32,  95,   0,   0, 127,   0, 127, 255, palette );  // half blue -> teal
        colorGradient(  96, 159,   0, 127, 255,   0, 255,   0, palette );  // teal -> green
        colorGradient( 160, 191,   0, 255,   0, 255, 255, 255, palette );  // green -> white
        colorGradient( 192, 255, 255, 255, 255, 255, 255, 255, palette );  // white
        /*
        colorGradient(   0,  63,   1,   1,   1,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127,   0, 127, 255, palette );  // half blue -> teal
        colorGradient( 128, 191,   0, 127, 255,   0, 255,   0, palette );  // teal -> green
        colorGradient( 192, 255,   0, 255,   0, 255, 255, 255, palette );  // green -> white
        //*/
        break;
    }
}

/*
void drawLine(int x1, int y1, int x2, int y2, int width, int height, uint8_t pixels[]) {

}
//*/

void drawBox(int x1, int y1, int x2, int y2, uint8_t color, int width, int height, uint8_t pixels[]) {

    // draw horizontal lines
    for (int x=x1; x <= x2; x++) {
        pixels[ (y1 * width) + x ] = color;
        pixels[ (y2 * width) + x ] = color;
    }
    // draw vertical lines
    for (int y=y1; y <= y2; y++) {
        pixels[ (y * width) + x1 ] = color;
        pixels[ (y * width) + x2 ] = color;
    }
}

void drawRandomBox(int width, int height, uint8_t pixels[]) {

    int x1 = randomInt(0, width - 2);
    int y1 = randomInt(0, height - 2);
    int x2 = randomInt(x1, width - 1);
    int y2 = randomInt(y1, height - 1);
    uint8_t color = 0xFF;
    drawBox(x1, y1, x2, y2, color, width, height, pixels);
}

void drawRandomWave(int width, int height, uint8_t pixels[]) {

    int y, wave = 0;
    int hh = height >> 1;

    for (int x=0; x < width; x++) {
        wave += randomInt(-1, +1);
        y = hh + wave;
        if ((y < 0) || (y >= height)) { y = hh; }
        pixels[ (y * width) + x ] = 0xFF;
    }
}

void blur1(int width, int height, uint8_t pixels[]) {

    int size = width * (height - 1) - 1;
    uint8_t dot;
    // blur effect
    for (int i=0; i < size; i++) {
        dot = (uint8_t)((pixels[i] + pixels[i + 1] + pixels[i + width] + pixels[i + width + 1]) >> 2) & 0xFF;
        if (dot <= 8) { dot = 0; }
        else { dot -= 8; }
        pixels[i] = dot;
    }
}

// NOT USED
void blur2(int width, int height, uint8_t pixels[]) {

    int size = width * (height - 1) - 1;
    uint8_t dot;
    // blur effect
    for (int i=0; i < size; i++) {
        dot = (uint8_t)(( (pixels[i] << 2) +
            (pixels[i] + pixels[i + 1] + pixels[i + width] + pixels[i + width + 1]) ) >> 3) & 0xFF;
        if (dot > 0) { dot--; }
        pixels[i] = dot;
    }
}

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // will use default if not set otherwise
    if (argc > 1) {
        hostname = argv[1];        // hostname can be supplied as first arg
    }

    srandom(time(NULL)); // seed the random generator

    int width = DISPLAY_WIDTH;
    int height = DISPLAY_HEIGHT;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, width, height);
    canvas.Clear();

    // set the color palette
    Color palette[256];

    // pixel buffer
    uint8_t pixels[ width * height ];
    for (int i=0; i < width * height; i++) { pixels[i] = 0; }  // clear pixel buffer

    int count = 0;
    int curPalette = 0;

    while (1) {

        // set new color palette
        if ((count % 100) == 0) {
            setPalette(curPalette, palette);
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 0; }
        }

        if ((count % 2) == 0) {
            switch (DEMO) {
                case 0: drawRandomWave(width, height, pixels); break;
                case 1: drawRandomBox(width, height, pixels); break;
            }
        }

        // draw black border & blur on every frame
        drawBox(0, 0, width-1, height-1, 0, width, height, pixels);
        blur1(width, height, pixels);

        // copy pixel buffer to canvas
        int dst = 0;
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                canvas.SetPixel( x, y, palette[ pixels[dst] ] );
                dst++;
            }
        }

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();
        usleep(DELAY * 1000);

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
