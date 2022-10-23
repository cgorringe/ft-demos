// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// blur
// Copyright (c) 2016-2020 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Displays boxes or bolts with blur effect.
//
// How to run:
//
// To see command line options:
//  ./blur -?
//  ./blur bolt
//  ./blur boxes
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./blur -h localhost
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
#define Z_LAYER 1      // (0-15) 0=background
#define DELAY 50
#define PALETTE_MAX 3  // 1=Nebula, 2=Fire, 3=Bluegreen
#define DEMO 0         // 0=bolt, 1=boxes, 2=circles, 3=target, 4=fire

const int kDemoBolt = 0;
const int kDemoBoxes = 1;
const int kDemoCircles = 2;
const int kDemoTarget = 3;
const int kDemoFire = 4;
const int kDemoAll = 5; // always make this +1 after last

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
int opt_palette = -1;  // default cycles
int opt_demo = DEMO;
int opt_orient = 0;

int usage(const char *progname) {

    fprintf(stderr, "Blur (c) 2016-2020 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options] {all|bolt|boxes|circles|target|fire}\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 50)\n"
        "\t-p <palette>   : Set color palette to: (default cycles)\n"
        "\t                  1=Nebula, 2=Fire, 3=Bluegreen\n"
        "\t-o <orient>    : Set orientation: 0=default, 1=XY-swapped\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:p:o:")) != -1) {
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
        case 'p':  // color palette
            if (sscanf(optarg, "%d", &opt_palette) != 1 || opt_palette < 1 || opt_palette > PALETTE_MAX) {
                fprintf(stderr, "Invalid color palette '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 'o':  // orientation
            if (sscanf(optarg, "%d", &opt_orient) != 1 || opt_orient < 0 || opt_orient > 1) {
                fprintf(stderr, "Invalid orientation '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        default:
            return usage(argv[0]);
        }
    }

    // retrieve arg text
    const char *text = argv[optind];
    if (text && strncmp(text, "all", 3) == 0) {
        opt_demo = kDemoAll;
    }
    else if (text && strncmp(text, "bolt", 4) == 0) {
        opt_demo = kDemoBolt;
    }
    else if (text && strncmp(text, "boxes", 5) == 0) {
        opt_demo = kDemoBoxes;
    }
    else if (text && strncmp(text, "circles", 7) == 0) {
        opt_demo = kDemoCircles;
    }
    else if (text && strncmp(text, "target", 6) == 0) {
        opt_demo = kDemoTarget;
    }
    else if (text && strncmp(text, "fire", 4) == 0) {
        opt_demo = kDemoFire;
    }
    else {
        fprintf(stderr, "Missing 'bolt', 'boxes', etc...\n");
        return usage(argv[0]);
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

void setPalette(int num, Color palette[]) {
    switch (num) {
      case 0:
        break;
      case 1:
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
      case 2:
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
      case 3:
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

// use this to make sure pixel is within bounds
void setPixel(int x0, int y0, uint8_t color, int width, int height, uint8_t pixels[]) {

    if ((x0 >= 0) && (x0 < width) && (y0 >= 0) && (y0 < height)) {
        pixels[ y0 * width + x0] = color;
    }
}

void drawCircle(int x0, int y0, int radius, uint8_t color, int width, int height, uint8_t pixels[]) {

    // based off code from ft/api/lib/graphics.cc
    int x = radius, y = 0;
    int radiusError = 1 - x;

    while (y <= x) {
        setPixel(  x + x0,  y + y0, color, width, height, pixels );
        setPixel(  y + x0,  x + y0, color, width, height, pixels );
        setPixel( -x + x0,  y + y0, color, width, height, pixels );
        setPixel( -y + x0,  x + y0, color, width, height, pixels );
        setPixel( -x + x0, -y + y0, color, width, height, pixels );
        setPixel( -y + x0, -x + y0, color, width, height, pixels );
        setPixel(  x + x0, -y + y0, color, width, height, pixels );
        setPixel(  y + x0, -x + y0, color, width, height, pixels );
        y++;
        if (radiusError < 0) {
            radiusError += 2 * y + 1;
        } 
        else {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}

void drawRandomCircle(int width, int height, uint8_t pixels[]) {

    int x0 = randomInt(0, width - 2);
    int y0 = randomInt(0, height - 2);
    int radius = randomInt(2, width / 3);
    uint8_t color = 0xFF;
    drawCircle(x0, y0, radius, color, width, height, pixels);
}

void drawRandomTarget(int width, int height, uint8_t pixels[]) {

    int x0 = width / 2;
    int y0 = width / 2;
    int radius = randomInt(2, width / 2);
    uint8_t color = 0xFF;
    drawCircle(x0, y0, radius, color, width, height, pixels);
}

void drawRandomBolt(int width, int height, uint8_t pixels[]) {

    int y, wave = 0;
    int hh = height >> 1;

    for (int x=0; x < width; x++) {
        wave += randomInt(-1, +1);
        y = hh + wave;
        if ((y < 0) || (y >= height)) { y = hh; }
        pixels[ (y * width) + x ] = 0xFF;
    }
}

// Draw random dots along bottom row.
void drawRandomFire(int width, int height, int orient, uint8_t pixels[]) {

    const uint8_t color = 0xFF;
    int num;
    // draw random dots
    if (orient == 0) {
        // flow upwards
        num = randomInt(1, width-2);
        for (int i=0; i < num; i++) {
            // upwards
            int x = randomInt(1, width-2);
            int y = height - 1;
            pixels[ (y * width) + x ] = color;
        }
    }
    else {
        // flow leftwards
        num = randomInt(1, height-2);
        for (int i=0; i < num; i++) {
            // leftwards
            int x = width - 1;
            int y = randomInt(1, height-2);
            pixels[ (y * width) + x ] = color;
        }
    }
}

void clearBottomRow(int width, int height, int orient, uint8_t pixels[]) {

    if (orient == 0) {
        // clear bottom row
        int by = (height-1) * width;
        for (int x=0; x < width; x++) {
            pixels[ by + x ] = 0;
        }
    }
    else {
        // clear right column
        int bx = width-1;
        for (int y=0; y < height; y++) {
            pixels[ (y * width) + bx ] = 0;
        }
    }
}

// This one works, but requires the following be called prior which results in a black right & bottom border:
// drawBox(0, 0, opt_width-1, opt_height-1, 0, opt_width, opt_height, pixels);

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

// Blur that works without the black border. Use this one.
void blur3(int width, int height, uint8_t pixels[]) {

    // blur effect
    uint8_t dot;
    int i=0;
    for (int y=0; y < height - 1; y++) {
        for (int x=0; x < width - 1; x++) {
            dot = (uint8_t)((pixels[i] + pixels[i + 1] + pixels[i + width] + pixels[i + width + 1]) >> 2) & 0xFF;
            dot = (dot <= 8) ? 0 : dot - 8;
            pixels[i] = dot;
            i++;
        }
        // blur right border pixel
        dot = (uint8_t)((pixels[i] + pixels[i + width]) >> 2) & 0xFF;
        dot = (dot <= 8) ? 0 : dot - 8;
        pixels[i] = dot;
        i++;
    }
    // blur bottom border pixels
    for (int x=0; x < width - 1; x++) {
        dot = (uint8_t)((pixels[i] + pixels[i + 1]) >> 2) & 0xFF;
        dot = (dot <= 8) ? 0 : dot - 8;
        pixels[i] = dot;
        i++;
    }
    // last lower-right corner pixel
    pixels[i] = 0;
}

// Blur for fire effect.
void blurFire(int width, int height, int orient, uint8_t pixels[]) {

    const int step = 4;
    int size = width * (height - 1) - 1;
    uint8_t dot;

    // TODO: redo this like blur3() to handle right border
    if (orient == 0) {
        // flame upwards (default orientation)
        for (int i=1; i < size; i++) {
            dot = (uint8_t)(( pixels[i - 1] + pixels[i + 1] + pixels[i + width - 1] + pixels[i + width]
                + pixels[i + width + 1] + pixels[i + 2*width - 1] + pixels[i + 2*width] + pixels[i + 2*width + 1]
                ) >> 3) & 0xFF;
            if (dot <= step) { dot = 0; } else { dot -= step; }
            pixels[i] = dot;
        }
    }
    else {
        // flame leftwards (orient = 1)
        for (int i=1; i < size; i++) {
            if (i % width == 0) continue;
            dot = (uint8_t)(( pixels[i - 1] + pixels[i] + pixels[i + 1] + pixels[i + width]
                + pixels[i + width + 1] + pixels[i + 2*width - 1] + pixels[i + 2*width] + pixels[i + 2*width + 1]
                ) >> 3) & 0xFF;
            if (dot <= step) { dot = 0; } else { dot -= step; }
            pixels[i + width - 1] = dot;
        }
    }
}

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    srandom(time(NULL)); // seed the random generator

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // pixel buffer
    uint8_t pixels[ opt_width * opt_height ];
    for (int i=0; i < opt_width * opt_height; i++) { pixels[i] = 0; }  // clear pixel buffer

    // color palette
    Color palette[256];
    int curPalette = (opt_palette < 0) ? 1 : opt_palette;
    setPalette(curPalette, palette);

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // other vars
    int count = 1;
    time_t starttime = time(NULL);
    int curDemo = (opt_demo == kDemoAll) ? 0 : opt_demo;

    do {
        // set new color palette
        if ( ((count % 100) == 0) && (opt_palette < 0) ) {
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 1; }
            setPalette(curPalette, palette);
        }

        // cycle all demos
        if ( (opt_demo == kDemoAll) && ((count % 300) == 0) ) {
            curDemo++;
            if (curDemo >= kDemoAll) { curDemo = 0; }
        }

        if ((count % 2) == 0) {
            switch (curDemo) {
                case kDemoBolt: drawRandomBolt(opt_width, opt_height, pixels); break;
                case kDemoBoxes: drawRandomBox(opt_width, opt_height, pixels); break;
                case kDemoCircles: drawRandomCircle(opt_width, opt_height, pixels); break;
                case kDemoTarget: drawRandomTarget(opt_width, opt_height, pixels); break;
            }
        }

        // blur on every frame
        if (curDemo == kDemoFire) {
            drawRandomFire(opt_width, opt_height, opt_orient, pixels);
            blurFire(opt_width, opt_height, opt_orient, pixels);
            clearBottomRow(opt_width, opt_height, opt_orient, pixels);
        }
        else {
            blur3(opt_width, opt_height, pixels);
        }

        // copy pixel buffer to canvas
        int dst = 0;
        for (int y=0; y < opt_height; y++) {
            for (int x=0; x < opt_width; x++) {
                canvas.SetPixel( x, y, palette[ pixels[dst] ] );
                dst++;
            }
        }

        // send canvas
        canvas.SetOffset(opt_xoff + DISPLAY_XOFF, opt_yoff + DISPLAY_YOFF, opt_layer);
        canvas.Send();
        usleep(opt_delay * 1000);

        count++;
        if (count == INT_MAX) { count=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received );

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    if (interrupt_received) return 1;
    return 0;
}
