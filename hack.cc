// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// hack
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/8/2016
//
// Displays rotating letters with blur effect.
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
//  ./hack -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./hack
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
#include "hack_font.h"

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <string>
#include <signal.h>

// Defaults                      large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 7      // (0-15) 0=background
#define DELAY 100
#define PALETTE_MAX 3  // 1=Nebula, 2=Fire, 3=Bluegreen
#define BLUR_DROP 32  // 8, 16

#define DISPLAY_TEXT "HACK"
#define TEXT_LENGTH 100

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
int opt_palette = -1;  // default cycles
int opt_repeat  = -1;  // default never ends (until timeout)
char opt_display_text[TEXT_LENGTH];

int usage(const char *progname) {

    fprintf(stderr, "Hack (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options] <text>...\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 25)\n"
        "\t-p <palette>   : Set color palette to: (default cycles)\n"
        "\t                  1=Nebula, 2=Fire, 3=Bluegreen\n"
        "\t-r <repeat>    : Repeat phrase x number of times, then exits. (default never ends)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:p:r:")) != -1) {
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
        case 'r':  // repeat
            if (sscanf(optarg, "%d", &opt_repeat) != 1 || opt_repeat < 0) {
                fprintf(stderr, "Invalid repeat value '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        default:
            return usage(argv[0]);
        }
    }

    // assign default DISPLAY_TEXT
    strncpy(opt_display_text, DISPLAY_TEXT, TEXT_LENGTH);

    // retrieve arg text from remaining arguments
    std::string str;
    if (argv[optind]) {
        str.append(argv[optind]);
        for (int i = optind + 1; i < argc; i++) {
            str.append(" ").append(argv[i]);
        }
        const char *text = str.c_str();
        while (isspace(*text)) { text++; }  // remove leading spaces
        if (text && (strlen(text) > 0)) {
            strncpy(opt_display_text, text, TEXT_LENGTH);
        }
    }
    printf("'%s'\n", opt_display_text);  // DEBUG

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
        colorGradient(   0,  63,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127, 127,   0, 255, palette );  // half blue -> blue-violet
        colorGradient( 128, 191, 127,   0, 255, 255,   0,   0, palette );  // blue-violet -> red
        colorGradient( 192, 255, 255,   0,   0, 255, 255, 255, palette );  // red -> white
        break;
    case 2:
        // Fire
        colorGradient(   0,  63,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127, 255,   0,   0, palette );  // half blue -> red
        colorGradient( 128, 191, 255,   0,   0, 255, 255,   0, palette );  // red -> yellow
        colorGradient( 192, 255, 255, 255,   0, 255, 255, 255, palette );  // yellow -> white
        break;
    case 3:
        // Bluegreen
        colorGradient(   0,  63,   0,   0,   0,   0,   0, 127, palette );  // black -> half blue
        colorGradient(  64, 127,   0,   0, 127,   0, 127, 255, palette );  // half blue -> teal
        colorGradient( 128, 191,   0, 127, 255,   0, 255,   0, palette );  // teal -> green
        colorGradient( 192, 255,   0, 255,   0, 255, 255, 255, palette );  // green -> white
        break;
    }
}

// Bresenham's line algorithm
void drawLine(int x1, int y1, int x2, int y2, uint8_t color, int width, int height, uint8_t pixels[]) {

    int xinc1, xinc2, yinc1, yinc2, den, num, numadd, numpixels;
    int deltax = abs(x2 - x1);
    int deltay = abs(y2 - y1);
    int x = x1; 
    int y = y1;

    if (x2 >= x1) { xinc1 =  1; xinc2 =  1; }
    else          { xinc1 = -1; xinc2 = -1; }

    if (y2 >= y1) { yinc1 =  1; yinc2 =  1; }
    else          { yinc1 = -1; yinc2 = -1; }

    if (deltax >= deltay) {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    }
    else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (int curpixel = 0; curpixel <= numpixels; curpixel++) {
        if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
            pixels[ (y * width) + x ] = color;
        }
        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
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

void drawRandomLine(int width, int height, uint8_t pixels[]) {

    int x1 = randomInt(0, width  - 1);
    int y1 = randomInt(0, height - 1);
    int x2 = randomInt(0, width  - 1);
    int y2 = randomInt(0, height - 1);
    uint8_t color = 0xFF;
    drawLine(x1, y1, x2, y2, color, width, height, pixels);
}

void blur(int width, int height, uint8_t pixels[]) {
    
    int size = width * (height - 1) - 1;
    uint8_t dot;
    // blur effect
    for (int i=0; i < size; i++) {
        dot = (uint8_t)((pixels[i] + pixels[i + 1] + pixels[i + width] + pixels[i + width + 1]) >> 2) & 0xFF;
        if (dot <= BLUR_DROP) { dot = 0; }
        else { dot -= BLUR_DROP; }
        pixels[i] = dot;
    }
}

void drawHackChar(int charcode, int angle, uint8_t color, int width, int height, uint8_t pixels[]) {

    int hw = (width >> 1);
    int hh = (height >> 1);
    int x1, y1, x2, y2;
    float sx1, sy1, sz1, sx2, sy2, sz2;
    int px1, py1, px2, py2;
    float cs, sn;
    int i=0;
    int D=32, Z=15;  // distance & z-factor

    cs = cos( angle * 3.14159 / 180 );
    sn = sin( angle * 3.14159 / 180 );

    x1 = hackfont[charcode][i][0];
    y1 = hackfont[charcode][i][1];
    x2 = hackfont[charcode][i][2];
    y2 = hackfont[charcode][i][3];

    while (!((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0))) {
        // scale and rotate x and y
        sx1 = (x1 * cs);
        sy1 = y1;
        sz1 = x1 * sn + Z;
        sx2 = (x2 * cs);
        sy2 = y2;
        sz2 = x2 * sn + Z;

        if (sz1 == 0) { sz1 = 1; }
        if (sz2 == 0) { sz2 = 1; }

        // project 3-D to 2-D space
        px1 = (int)(D * sx1/sz1);
        py1 = (int)(D * sy1/sz1);
        px2 = (int)(D * sx2/sz2);
        py2 = (int)(D * sy2/sz2);

        drawLine(px1 + hw, py1 + hh, px2 + hw, py2 + hh, color, width, height, pixels);

        // next line in polygon
        i++;
        x1 = hackfont[charcode][i][0];
        y1 = hackfont[charcode][i][1];
        x2 = hackfont[charcode][i][2];
        y2 = hackfont[charcode][i][3];
    }
}

// NOT USED
/*
void toUpper(char text[]) {
    int diff = abs('a' - 'A');
    int i=0;
    while (char c = text[i]) {
        if ((c >= 'a') && (c <= 'z')) {
            text[i] -= diff;
        }
        i++;
    }
}
//*/

void convertTextToCodes(const char intext[], int outcodes[] ) {

    int length = strlen(intext);
    char c;
    int dst=0;

    for (int src=0; src < length; src++) {
        c = intext[src];
        if ((c >= '0') && (c <= '9')) {
            outcodes[dst++] = c - '0';
        }
        else if ((c >= 'A') && (c <= 'Z')) {
            outcodes[dst++] = c - 'A' + 10;
        }
        else if ((c >= 'a') && (c <= 'z')) {
            outcodes[dst++] = c - 'a' + 10;
        }
    }
    outcodes[dst] = -1;
}

// --------------------------------------------------------------------------------

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

    // prepare text codes
    int textcodes[80], charcount=0;
    const char *text = opt_display_text;
    convertTextToCodes(text, textcodes);

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // other vars
    int count=0, angle=0;
    time_t starttime = time(NULL);

    do {
        // set new color palette
        if ( ((count % 200) == 0) && (opt_palette < 0) ) {
            setPalette(curPalette, palette);
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 1; }
        }

        // draw black border & blur on every frame
        drawBox(0, 0, opt_width-1, opt_height-1, 0, opt_width, opt_height, pixels);
        blur(opt_width, opt_height, pixels);

        // draw random lines (TEST)
        //if ((count % 1) == 0) {
        //    drawRandomLine(opt_width, opt_height, pixels);
        //}

        // draw rotating letter
        if ((count % 1) == 0) {
            //for (int i=0; i < opt_width * opt_height; i++) { pixels[i] = 0; }  // clear pixel buffer
            //drawHackChar( textcodes[charcount], angle, 0x00, opt_width, opt_height, pixels );
            angle += 8;
            if (angle > 360) { angle -= 360; }
            drawHackChar( textcodes[charcount], angle, 0xFF, opt_width, opt_height, pixels );
        }
        if ((count % 45) == 40) {  // TODO
            charcount++;
            if (textcodes[charcount] == -1) { 
                // back to start of text
                charcount = 0;
                if (opt_repeat > 0) { 
                    opt_repeat--;
                    if (opt_repeat == 0) { break; }
                }
            }
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
        canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
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
