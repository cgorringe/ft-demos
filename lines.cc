// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// lines
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 5/2/2016
//
// Draws lines that bounce off the walls and smoothly transition between colors.
//
// Based on my Lines! Screensaver App:
// http://carl.gorringe.org/#lines
//
// How to run:
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./lines localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./lines
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
#include <math.h>
#include <time.h>

//                               large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 11      // (0-15) 0=background
#define DELAY 150

#define NUM_LINES 6
#define SKIP_MIN 2
#define SKIP_MAX 3
#define LINE_ALGO 1  // 0=dots, 1=plain line, 2=anti-aliased line
#define DRAW_FOUR 0  // 0=one line, 1=four lines

#define TRUE 1
#define FALSE 0

struct Line {
    Line() {}
    Line(int xx1, int yy1, int xx2, int yy2) : x1(xx1), y1(yy1), x2(xx2), y2(yy2) { }

    int x1;
    int y1;
    int x2;
    int y2;
};

Line lines[NUM_LINES];
int lines_idx;


// random int in range min to max inclusive
int randomInt(int min, int max) {
  //return (arc4random_uniform(max - min + 1) + min);
  return (random() % (max - min + 1) + min);
}

// draw endpoints of line
void drawLine0(int x1, int y1, int x2, int y2, const Color &color, UDPFlaschenTaschen &canvas) {
    canvas.SetPixel(x1, y1, color);
    canvas.SetPixel(x2, y2, color);
}

// ------------------------------------------------------------------------------------------
// Bresenham's line algorithm

void drawLine1(int x1, int y1, int x2, int y2, const Color &color, UDPFlaschenTaschen &canvas) {

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
        canvas.SetPixel(x, y, color);
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

// ------------------------------------------------------------------------------------------
// Xiaolin Wu's anti-aliased line algorithm
// http://rosettacode.org/wiki/Xiaolin_Wu%27s_line_algorithm#C

#define ipart_(X) ((int)(X))
#define round_(X) ((int)(((double)(X))+0.5))
#define fpart_(X) (((double)(X))-(double)ipart_(X))
#define rfpart_(X) (1.0-fpart_(X))
#define swap_(a, b) do{ __typeof__(a) tmp; tmp = a; a = b; b = tmp; }while(0)

void drawLine2(int x1, int y1, int x2, int y2, const Color &color, UDPFlaschenTaschen &canvas) {

  double br1, br2;
  Color color1, color2;
  double dx = (double)x2 - (double)x1;
  double dy = (double)y2 - (double)y1;
  if ( fabs(dx) > fabs(dy) ) {
    if ( x2 < x1 ) {
      swap_(x1, x2);
      swap_(y1, y2);
    }
    double gradient = dy / dx;
    double xend = round_(x1);
    double yend = y1 + gradient * (xend - x1);
    double xgap = rfpart_(x1 + 0.5);
    int xpxl1 = xend;
    int ypxl1 = ipart_(yend);
    //plot_(xpxl1, ypxl1, rfpart_(yend)*xgap);
    //plot_(xpxl1, ypxl1+1, fpart_(yend)*xgap);
    br1 = rfpart_(yend) * xgap;
    br2 = fpart_(yend) * xgap;
    color1 = Color(color.r * br1, color.g * br1, color.b * br1);
    color2 = Color(color.r * br2, color.g * br2, color.b * br2);
    canvas.SetPixel(xpxl1, ypxl1, color1);
    canvas.SetPixel(xpxl1, ypxl1 + 1, color2);

    double intery = yend + gradient;
 
    xend = round_(x2);
    yend = y2 + gradient * (xend - x2);
    xgap = fpart_(x2 + 0.5);
    int xpxl2 = xend;
    int ypxl2 = ipart_(yend);
    //plot_(xpxl2, ypxl2, rfpart_(yend) * xgap);
    //plot_(xpxl2, ypxl2 + 1, fpart_(yend) * xgap);
    br1 = rfpart_(yend) * xgap;
    br2 = fpart_(yend) * xgap;
    color1 = Color(color.r * br1, color.g * br1, color.b * br1);
    color2 = Color(color.r * br2, color.g * br2, color.b * br2);
    canvas.SetPixel(xpxl2, ypxl2, color1);
    canvas.SetPixel(xpxl2, ypxl2 + 1, color2);

    for (int x = xpxl1 + 1; x <= (xpxl2 - 1); x++) {
      //plot_(x, ipart_(intery), rfpart_(intery));
      //plot_(x, ipart_(intery) + 1, fpart_(intery));
      br1 = rfpart_(intery);
      br2 = fpart_(intery);
      color1 = Color(color.r * br1, color.g * br1, color.b * br1);
      color2 = Color(color.r * br2, color.g * br2, color.b * br2);
      canvas.SetPixel(x, ipart_(intery), color1);
      canvas.SetPixel(x, ipart_(intery) + 1, color2);
      intery += gradient;
    }
  } else {
    if ( y2 < y1 ) {
      swap_(x1, x2);
      swap_(y1, y2);
    }
    double gradient = dx / dy;
    double yend = round_(y1);
    double xend = x1 + gradient * (yend - y1);
    double ygap = rfpart_(y1 + 0.5);
    int ypxl1 = yend;
    int xpxl1 = ipart_(xend);
    //plot_(xpxl1, ypxl1, rfpart_(xend)*ygap);
    //plot_(xpxl1, ypxl1+1, fpart_(xend)*ygap);
    br1 = rfpart_(xend) * ygap;
    br2 = fpart_(xend) * ygap;
    color1 = Color(color.r * br1, color.g * br1, color.b * br1);
    color2 = Color(color.r * br2, color.g * br2, color.b * br2);
    canvas.SetPixel(xpxl1, ypxl1, color1);
    canvas.SetPixel(xpxl1, ypxl1 + 1, color2);

    double interx = xend + gradient;
 
    yend = round_(y2);
    xend = x2 + gradient * (yend - y2);
    ygap = fpart_(y2 + 0.5);
    int ypxl2 = yend;
    int xpxl2 = ipart_(xend);
    //plot_(xpxl2, ypxl2, rfpart_(xend) * ygap);
    //plot_(xpxl2, ypxl2 + 1, fpart_(xend) * ygap);
    br1 = rfpart_(xend) * ygap;
    br2 = fpart_(xend) * ygap;
    color1 = Color(color.r * br1, color.g * br1, color.b * br1);
    color2 = Color(color.r * br2, color.g * br2, color.b * br2);
    canvas.SetPixel(xpxl2, ypxl2, color1);
    canvas.SetPixel(xpxl2, ypxl2 + 1, color2);

    for (int y = ypxl1 + 1; y <= (ypxl2 - 1); y++) {
      //plot_(ipart_(interx), y, rfpart_(interx));
      //plot_(ipart_(interx) + 1, y, fpart_(interx));
      br1 = rfpart_(interx);
      br2 = fpart_(interx);
      color1 = Color(color.r * br1, color.g * br1, color.b * br1);
      color2 = Color(color.r * br2, color.g * br2, color.b * br2);
      canvas.SetPixel(ipart_(interx), y, color1);
      canvas.SetPixel(ipart_(interx) + 1, y, color2);
      interx += gradient;
    }
  }
}


// ------------------------------------------------------------------------------------------

void drawLine(int x1, int y1, int x2, int y2, const Color &color, UDPFlaschenTaschen &canvas) {

    switch (LINE_ALGO) {
        case 0: drawLine0(x1, y1, x2, y2, color, canvas); break;
        case 1: drawLine1(x1, y1, x2, y2, color, canvas); break;
        case 2: drawLine2(x1, y1, x2, y2, color, canvas); break;
    }
}

Color nextColor(int reset) {

  static int count = 0;
  static int oldR = 0, oldG = 0, oldB = 0;
  static int newR = 0, newG = 0, newB = 0;
  static int skpR = 0, skpG = 0, skpB = 0;
  static int curR = 0, curG = 0, curB = 0;

  if (reset) { count = 0; }

  count--;
  if (count < 0) {
    count = 15;
    oldR = newR; oldG = newG; oldB = newB;

    do {
      // loop only if new color is black
      newR = (int)( randomInt(0, 1) * 255 );
      newG = (int)( randomInt(0, 1) * 255 );
      newB = (int)( randomInt(0, 1) * 255 );
    } while ((newR == 0) && (newG == 0) && (newB == 0));

    skpR = (newR == oldR) ? 0 : ((newR > oldR) ? 16 : -16);
    skpG = (newG == oldG) ? 0 : ((newG > oldG) ? 16 : -16);
    skpB = (newB == oldB) ? 0 : ((newB > oldB) ? 16 : -16);
    curR = oldR;
    curG = oldG;
    curB = oldB;
  }
  curR += skpR;
  curG += skpG;
  curB += skpB;

  return Color( (curR/256.0f)*255, (curG/256.0f)*255, (curB / 256.0f)*255 );
}

Line nextLine(int reset) {

  // global: lines_idx, lines[]
  static Line skip;

  int old_idx = lines_idx;
  lines_idx++;
  if (lines_idx >= NUM_LINES) { lines_idx = 0; }

  if (reset) {
    // randomly position a new line
    lines[lines_idx].x1 = randomInt(1, DISPLAY_WIDTH - 2);
    lines[lines_idx].y1 = randomInt(1, DISPLAY_HEIGHT - 2);
    lines[lines_idx].x2 = randomInt(1, DISPLAY_WIDTH - 2);
    lines[lines_idx].y2 = randomInt(1, DISPLAY_HEIGHT - 2);

    // random skip values
    skip.x1 = (randomInt(0, 1)) ? randomInt(SKIP_MIN, SKIP_MAX) : -randomInt(SKIP_MIN, SKIP_MAX);
    skip.y1 = (randomInt(0, 1)) ? randomInt(SKIP_MIN, SKIP_MAX) : -randomInt(SKIP_MIN, SKIP_MAX);
    skip.x2 = (randomInt(0, 1)) ? randomInt(SKIP_MIN, SKIP_MAX) : -randomInt(SKIP_MIN, SKIP_MAX);
    skip.y2 = (randomInt(0, 1)) ? randomInt(SKIP_MIN, SKIP_MAX) : -randomInt(SKIP_MIN, SKIP_MAX);
  }
  else {
    // move the line
    lines[lines_idx].x1 = lines[old_idx].x1 + skip.x1;
    lines[lines_idx].y1 = lines[old_idx].y1 + skip.y1;
    lines[lines_idx].x2 = lines[old_idx].x2 + skip.x2;
    lines[lines_idx].y2 = lines[old_idx].y2 + skip.y2;

    // reverse direction of step values of points that hit border
    if (lines[lines_idx].x1 <= 0)              { skip.x1 = randomInt(SKIP_MIN, SKIP_MAX);      }
    if (lines[lines_idx].x1 >= DISPLAY_WIDTH)  { skip.x1 = randomInt(SKIP_MIN, SKIP_MAX) * -1; }
    if (lines[lines_idx].y1 <= 0)              { skip.y1 = randomInt(SKIP_MIN, SKIP_MAX);      }
    if (lines[lines_idx].y1 >= DISPLAY_HEIGHT) { skip.y1 = randomInt(SKIP_MIN, SKIP_MAX) * -1; }
    if (lines[lines_idx].x2 <= 0)              { skip.x2 = randomInt(SKIP_MIN, SKIP_MAX);      }
    if (lines[lines_idx].x2 >= DISPLAY_WIDTH)  { skip.x2 = randomInt(SKIP_MIN, SKIP_MAX) * -1; }
    if (lines[lines_idx].y2 <= 0)              { skip.y2 = randomInt(SKIP_MIN, SKIP_MAX);      }
    if (lines[lines_idx].y2 >= DISPLAY_HEIGHT) { skip.y2 = randomInt(SKIP_MIN, SKIP_MAX) * -1; }
  }
  return lines[lines_idx];
}

Line lastLine() {
  // global: lines_idx, lines[]
  int last_idx = lines_idx + 1;
  if (last_idx >= NUM_LINES) { last_idx = 0; }
  return lines[last_idx];
}

void drawFourLines(const Line &line, const Color &color, UDPFlaschenTaschen &canvas) {

    drawLine( line.x1, line.y1, line.x2, line.y2, color, canvas);
    if (DRAW_FOUR) {
        drawLine( DISPLAY_WIDTH - line.x1, line.y1, DISPLAY_WIDTH - line.x2, line.y2, color, canvas);
        drawLine( line.x1, DISPLAY_HEIGHT- line.y1, line.x2, DISPLAY_HEIGHT - line.y2, color, canvas);
        drawLine( DISPLAY_WIDTH - line.x1, DISPLAY_HEIGHT - line.y1, 
                  DISPLAY_WIDTH - line.x2, DISPLAY_HEIGHT - line.y2, color, canvas);
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

    lines_idx = 0;
    Color transparent = Color(0, 0, 0);
    Color color = nextColor(TRUE);
    Line line = nextLine(TRUE);
    int count = 0;

    while (TRUE) {
        // erase last line
        drawFourLines(lastLine(), transparent, canvas);

        // draw colored line
        color = nextColor(FALSE);
        line = nextLine(FALSE);
        drawFourLines(line, color, canvas);

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();
        usleep(DELAY * 1000);

        count++;
        if (count == INT_MAX) { count=0; }
    }
}
