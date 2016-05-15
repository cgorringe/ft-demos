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

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

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

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 1      // (0-15) 0=background
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

    // We create a supersampling of our two-dimensional lookup-table. We
    // trade memory for CPU here.
    const int lookup_quant = 40;

    const int width = DISPLAY_WIDTH;
    const int height = DISPLAY_HEIGHT;

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, width, height);
    canvas.Clear();

    // set the color palette
    Color palette[256];

    // Value for pixels buffer
    Buffer2D<float> pixels(width, height);

    // Our plasma needs to cover double the area as we only look at
    // a window of it which we shift around.
    // This is essentially a two-dimensional lookup-table.
    Buffer2D<float> plasma1(lookup_quant * width * 2, lookup_quant * height * 2);
    Buffer2D<float> plasma2(lookup_quant * width * 2, lookup_quant * height * 2);
    const int center_x = lookup_quant * width;  // For our circular calcs.
    const int center_y = lookup_quant * height;
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

    const float slowness = 10;
    int x1, y1, x2, y2, x3, y3;

    // We slide a window of half the size within our plasma templates.
    const int hw = lookup_quant * width / 2;
    const int hh = lookup_quant * height / 2;

    int count = 0;
    int curPalette = 0;

    while (1) {
        // set new color palette
        if ((count % 2000) == 0) {
            setPalette(curPalette, palette);
            curPalette++;
            if (curPalette > PALETTE_MAX) { curPalette = 0; }
        }

        // Move plasma with sine functions
        x1 = hw + roundf(hw * cosf( count /  97.0f / slowness ));
        x2 = hw + roundf(hw * sinf(-count / 114.0f / slowness ));
        x3 = hw + roundf(hw * sinf(-count / 137.0f / slowness ));

        y1 = hh + roundf(hh * sinf( count / 123.0f / slowness ));
        y2 = hh + roundf(hh * cosf(-count /  75.0f / slowness ));
        y3 = hh + roundf(hh * cosf(-count / 108.0f / slowness ));

        float lowest_value = 100;   // Finding range below.
        float higest_value = -100;

        // Write plasma to pixel buffer, still as float. Keep track of range.
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
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
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                float value = pixels.At(x, y);
                // Normalize to [0..1]
                const float normalized = (value - lowest_value) / value_range;
                const uint8_t palette_entry = round(normalized * 255);
                canvas.SetPixel(x, y, Color(palette[palette_entry]));
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
