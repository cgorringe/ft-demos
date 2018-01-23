// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// plasma  (new version)
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Displays animated plasma effect on the Flaschen Taschen.
// This version uses anti-aliasing to smooth out jittering by 
// supersampling by 4x and down-sampling to the display resolution.
//
// How to run:
//
// To see command line options:
//  ./plasma -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./plasma -h localhost
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

#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <signal.h>

namespace {
// A two-dimensional array, essentially. A bit easier to use than manually
// calculating array positions.
template <class T> class Buffer2D {
public:
    Buffer2D(int width, int height) : width_(width), height_(height),
                                      screen_(new T [ width * height]) {
        bzero(screen_, width * height * sizeof(T));
    }

    ~Buffer2D() { delete [] screen_; }

    inline int width() const { return width_; }
    inline int height() const { return height_; }

    T &At(int x, int y) { return screen_[y * width_ + x]; }

private:
    const int width_;
    const int height_;
    T *const screen_;
};
}  // namespace

// Defaults                      large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 1      // (0-15) 0=background
#define DELAY 25              // Wait in ms. Determines frame rate.
#define MOVE_SLOWNESS 100.0   // Slowness of move. More for slow.

#define PALETTE_MAX 4  // 0=Rainbow, 1=Nebula, 2=Fire, 3=Bluegreen, 4=RGB


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

int usage(const char *progname) {

    fprintf(stderr, "Plasma (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 25)\n"
        "\t-p <palette>   : Set color palette to: (default cycles)\n"
        "\t                  0=Rainbow, 1=Nebula, 2=Fire, 3=Bluegreen, 4=RGB\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?l:t:g:h:d:p:")) != -1) {
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
            if (sscanf(optarg, "%d", &opt_palette) != 1 || opt_palette < 0 || opt_palette > PALETTE_MAX) {
                fprintf(stderr, "Invalid color palette '%s'\n", optarg);
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

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // We create a supersampling of our two-dimensional lookup-table. We
    // trade memory for CPU here.
    const int lookup_quant = 20;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // set the color palette
    Color palette[256];

    // Value for pixels buffer
    Buffer2D<float> pixels(opt_width, opt_height);

    // Our plasma needs to cover double the area as we only look at
    // a window of it which we shift around.
    // This is essentially a two-dimensional lookup-table.
    Buffer2D<float> plasma1(lookup_quant * opt_width * 2, lookup_quant * opt_height * 2);
    Buffer2D<float> plasma2(lookup_quant * opt_width * 2, lookup_quant * opt_height * 2);
    const int center_x = lookup_quant * opt_width;  // For our circular calcs.
    const int center_y = lookup_quant * opt_height;
    for (int y=0; y < plasma1.height(); y++) {
        for (int x=0; x < plasma1.width(); x++) {
            plasma1.At(x, y) = sin(sqrt((center_y-y)*(center_y-y) +
                                        (center_x-x)*(center_x-x))
                                   / (4 * lookup_quant));
            plasma2.At(x, y)
                = sin((4.0 * x / lookup_quant) / (37.0 + 15.0 * cos(y / (18.5 * lookup_quant))))
                * cos((4.0 * y / lookup_quant) / (31.0 + 11.0 * sin(x / (14.25 * lookup_quant))) );
        }
    }

    const float slowness = MOVE_SLOWNESS / opt_delay;
    int x1, y1, x2, y2, x3, y3;

    // We slide a window of half the size within our plasma templates.
    const int hw = lookup_quant * opt_width / 2;
    const int hh = lookup_quant * opt_height / 2;

    srandom(time(NULL));
    int count = random();   // Set to 0 for predictable start.
    if (count < 0) count = -count;

    int curPalette = (opt_palette < 0) ? 0 : opt_palette;
    setPalette(curPalette, palette);

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    time_t starttime = time(NULL);

    float lowest_value = 100;   // Finding range below.
    float higest_value = -100;

    do {
        // set new color palette
        if ( ((count % 2000) == 0) && (opt_palette < 0) ) {
            setPalette(curPalette, palette);
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 0; }
        }

        // Move plasma with sine functions
        x1 = hw + round(hw * cos( count /  97.0 / slowness ));
        x2 = hw + round(hw * sin(-count / 114.0 / slowness ));
        x3 = hw + round(hw * sin(-count / 137.0 / slowness ));

        y1 = hh + round(hh * sin( count / 123.0 / slowness ));
        y2 = hh + round(hh * cos(-count /  75.0 / slowness ));
        y3 = hh + round(hh * cos(-count / 108.0 / slowness ));

        // Write plasma to pixel buffer, still as float. Keep track of range.
        for (int y=0; y < opt_height; y++) {
            for (int x=0; x < opt_width; x++) {
                const float value
                    = plasma1.At(x1+lookup_quant*x, y1+lookup_quant*y)
                    + plasma2.At(x2+lookup_quant*x, y2+lookup_quant*y)
                    + plasma2.At(x3+lookup_quant*x, y3+lookup_quant*y);
                if (value < lowest_value) lowest_value = value;
                if (value > higest_value) higest_value = value;
                pixels.At(x, y) = value;
            }
        }

        // Copy pixel buffer to canvas, lookup_quantd accordingly.
        const float value_range = higest_value - lowest_value;
        for (int y=0; y < opt_height; y++) {
            for (int x=0; x < opt_width; x++) {
                float value = pixels.At(x, y);
                // Normalize to [0..1]
                const float normalized = (value - lowest_value) / value_range;
                const uint8_t palette_entry = round(normalized * 255);
                canvas.SetPixel(x, y, Color(palette[palette_entry]));
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
