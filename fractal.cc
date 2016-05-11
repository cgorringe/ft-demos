// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// fractal
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/9/2016
//
// Draws and zooms into a Mandelbrot fractal.
// Based on code from The Art of Demomaking by Alex J. Champandard
// http://flipcode.com/archives/The_Art_of_Demomaking-Issue_08_Fractal_Zooming.shtml
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./fractal localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./fractal
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
#include <math.h>
#include <string.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 1       // (0-15) 0=background
#define DELAY 20


// define the point in the complex plane to which we will zoom into
#define POINT_OR  -0.577816-9.31323E-10-1.16415E-10
#define POINT_OI  -0.631121-2.38419E-07+1.49012E-08

int display_width = DISPLAY_WIDTH;
int display_height = DISPLAY_HEIGHT;

// global variables used for calculating fractal
uint8_t *glob_frac1, *glob_frac2;
double glob_dr, glob_di, glob_pr, glob_pi, glob_sr, glob_si;
long glob_offs;

// --------------------------------------------------------------------------------
// Fractal functions

// init fractal computation (MUST REDO)
void startFractal(double sr, double si, double er, double ei) {
    // compute deltas for interpolation in complex plane
    //glob_dr = (er - sr) / 640.0f;
    //glob_di = (ei - si) / 400.0f;
    glob_dr = (er - sr) / (DISPLAY_WIDTH * 2.0f);
    glob_di = (ei - si) / (DISPLAY_HEIGHT * 2.0f);
    // remember start values
    glob_pr = sr;
    glob_pi = si;
    glob_sr = sr;
    glob_si = si;
    glob_offs = 0;
}

// compute 4 lines of fractal (MUST REDO)
void computeFractal() {
    if ((glob_offs + 1) >= (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)) {
        return;
    }
    //for (int j=0; j < 4; j++) {
    for (int j=0; j < 2; j++) {
        glob_pr = glob_sr;
        for (int i=0; i < (DISPLAY_WIDTH * 2); i++) {
            uint8_t c = 0;
            double vi = glob_pi, vr = glob_pr, nvi, nvr;
            // loop until distance is above 2, or counter hits limit
            while ((vr*vr + vi*vi < 4) && (c < 255)) {
                // compute Z(n+1) given Z(n)
                nvr = vr*vr - vi*vi + glob_pr;
                nvi = 2 * vi * vr + glob_pi;
                // that becomes Z(n)
                vi = nvi;
                vr = nvr;
                c++;
            }
            // store color
            glob_frac1[glob_offs] = c;
            glob_offs++;
            if (glob_offs  >= (DISPLAY_WIDTH * DISPLAY_HEIGHT * 4)) { return; }
            // interpolate X
            glob_pr += glob_dr;
        }
        // interpolate Y
        glob_pi += glob_di;
    }
}

// finished computation, swap buffers
void finishFractal() {
   uint8_t *tmp = glob_frac1;
   glob_frac1 = glob_frac2;
   glob_frac2 = tmp;
}

void zoomFractal( double z, uint8_t pixels[] ) {

    // z = 0.0 to 1.0
    int width = (int)((DISPLAY_WIDTH<<17)/(256.0f*(1+z)))<<8,
        height = (int)((DISPLAY_HEIGHT<<17)/(256.0f*(1+z)))<<8,
        startx = ((DISPLAY_WIDTH<<17)-width)>>1,
        starty = ((DISPLAY_HEIGHT<<17)-height)>>1,
        deltax = width / DISPLAY_WIDTH,
        deltay = height / DISPLAY_HEIGHT,
        px, py = starty;
    long glob_offs = 0;
    for (int j=0; j < DISPLAY_HEIGHT; j++) {
        px = startx;
        for (int i=0; i < DISPLAY_WIDTH; i++) {
            // bilinear filter
            pixels[glob_offs] =
                ( glob_frac2[(py>>16)*(DISPLAY_WIDTH * 2)+(px>>16)] * (0x100-((py>>8)&0xff)) * (0x100-((px>>8)&0xff))
                + glob_frac2[(py>>16)*(DISPLAY_WIDTH * 2)+((px>>16)+1)] * (0x100-((py>>8)&0xff)) * ((px>>8)&0xff)
                + glob_frac2[((py>>16)+1)*(DISPLAY_WIDTH * 2)+(px>>16)] * ((py>>8)&0xff) * (0x100-((px>>8)&0xff))
                + glob_frac2[((py>>16)+1)*(DISPLAY_WIDTH * 2)+((px>>16)+1)] * ((py>>8)&0xff) * ((px>>8)&0xff) ) >> 16;
            // interpolate X
            px += deltax;
            glob_offs++;
        }
        // interpolate Y
        py += deltay;
    }
}

void updatePalette(int t, Color palette[]) {

    //int tmpi=0;
    //double tmpf=0;
    uint8_t colr1, colr2;

    for (int i=0; i < 256; i++) {
        //colr1 = (int)(128.0f - 127.0f * cos( i * M_PI / 128.0f + (t * 0.00141f) )) & 0xFF;
        //colr2 = (int)(128.0f - 127.0f * cos( i * M_PI /  64.0f + (t * 0.0136f)  )) & 0xFF;
        colr1 = (int)(128.0f - 127.0f * cos( i * M_PI / 128.0f + (t * 0.0212f) )) & 0xFF;
        colr2 = (int)(128.0f - 127.0f * cos( i * M_PI /  64.0f + (t * 0.0136f)  )) & 0xFF;
        palette[i].r = colr2;  // colr1
        palette[i].g = 0;      // colr1
        palette[i].b = colr1;  // colr2
        //printf("(%d: %02x %02x %02x) ", i, palette[i].r, palette[i].g, palette[i].b);
        //tmpf = cos( i * M_PI / 128.0f + (t * 0.00141f) );
        //tmpi = 128 - 127 * tmpf;
        //printf("(%d: %.3f %d) ", i, tmpf, tmpi);
    }
    //printf("\n");
    //*/
}


// --------------------------------------------------------------------------------
// Main

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // will use default if not set otherwise
    if (argc > 1) {
        hostname = argv[1];        // hostname can be supplied as first arg
    }

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, display_width, display_height);
    canvas.Clear();

    // init vars
    Color palette[256];
    uint8_t pixels[ display_width * display_height ];
    for (int i=0; i < display_width * display_height; i++) { pixels[i] = 0; }  // clear pixel buffer
    int count=0;

    // setup the palette
    int k=0;
    // allocate memory for our fractal
    //glob_frac1 = new uint8_t[640 * 400];
    //glob_frac2 = new uint8_t[640 * 400];
    glob_frac1 = new uint8_t[DISPLAY_WIDTH * DISPLAY_HEIGHT * 4];
    glob_frac2 = new uint8_t[DISPLAY_WIDTH * DISPLAY_HEIGHT * 4];

    // set original zooming settings
    double zx = 4.0, zy = 4.0;
    bool zoom_in = true;
    // calculate the first fractal
    printf("Calculating first frame... ");
    startFractal( POINT_OR - zx, POINT_OI - zy, POINT_OR + zx, POINT_OI + zy );
    for (int j=0; j < 100; j++) { computeFractal(); }
    finishFractal();
    printf("done\n");
    
    updatePalette(0, palette);
    long long frameCount = 0;

    while (1) {

        // adjust zooming coefficient for next view
        if (zoom_in) { zx *= 0.5; zy *= 0.5; }
        else { zx *= 2; zy *= 2; }

        // start calculating the next fractal
        startFractal( POINT_OR - zx, POINT_OI - zy, POINT_OR + zx, POINT_OI + zy );
        int j=0;
        //while (j < 100) {
        while (j < (DISPLAY_HEIGHT * 2)) {
            j++;
            // calc another few lines
            computeFractal();

            // display the old fractal, zooming in or out
            //if (zoom_in) { zoomFractal( (double)j / 100.0f ); }
            //else { zoomFractal( 1.0f - (double)j / 100.0f ); }
            if (zoom_in) { zoomFractal( (double)j / (DISPLAY_HEIGHT * 2), pixels ); }
            else { zoomFractal( 1.0f - (double)j / (DISPLAY_HEIGHT * 2), pixels ); }

            // select some new colours
            //updatePalette( k * 100 + j );
            updatePalette( k * (DISPLAY_HEIGHT * 2) + j, palette );

            // dump to screen
            //vga->Update();

            // copy pixel buffer to canvas
            int dst = 0;
            for (int y=0; y < display_height; y++) {
                for (int x=0; x < display_width; x++) {
                    canvas.SetPixel( x, y, palette[ pixels[dst] ] );
                    dst++;
                }
            }

            // send canvas
            canvas.SetOffset(0, 0, Z_LAYER);
            canvas.Send();
            usleep(DELAY * 1000);

            frameCount++;
        }
        // one more image displayed
        k++;
        // check if we've gone far enough
        if (k % 38 == 0) {
            // if so, reverse direction
            zoom_in = !zoom_in;
            if (zoom_in) { zx *= 0.5; zy *= 0.5; }
            else { zx *= 2.0; zy *= 2.0; }

            // and make sure we use the same fractal again, in the other direction
            finishFractal();
        }
        finishFractal();

        count++;
        if (count == INT_MAX) { count=0; }
    }

    // not reached
    //delete glob_frac1;
    //delete glob_frac2;
}
