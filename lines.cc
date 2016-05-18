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
// To see command line options:
//  ./lines -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./lines -h localhost one
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./lines one
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <string>

// Defaults                      large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 3      // (0-15) 0=background
#define DELAY 50      // 150

#define MAX_LINES 50
#define NUM_LINES 6  // 6
#define SKIP_MIN 1
#define SKIP_MAX 3
#define LINE_ALGO 1  // 0=dots, 1=plain line, 2=anti-aliased line
#define DRAW_NUM 1   // 1, 2, or 4 lines

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

// global vars
Line lines[MAX_LINES];
int lines_idx;

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
int opt_delay  = DELAY;
int opt_draw_num = DRAW_NUM;    // 1, 2, or 4 lines
int opt_line_algo = LINE_ALGO;  // 0=dots, 1=plain line, 2=anti-aliased line
int opt_num_lines = NUM_LINES;
int opt_skip_min = SKIP_MIN;
int opt_skip_max = SKIP_MAX;

int usage(const char *progname) {

    fprintf(stderr, "Lines (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options] {one|two|four} \n", progname);
    fprintf(stderr, "Options:\n"
        "\t-l <layer>     : Layer 0-15. (default 3)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-g <W>x<H>     : Output geometry. (default 45x35)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 50)\n"
        "\t-a             : Anti-alias the lines.\n"
        "\t-n <number>    : Number of lines. (default 6)\n"
        "\t-s <min>,<max> : Skip min,max points. (default 1,3)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?l:t:g:h:d:an:s:")) != -1) {
        switch (opt) {
        case '?':  // help
            return usage(argv[0]);
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
        case 'g':  // geometry
            if (sscanf(optarg, "%dx%d", &opt_width, &opt_height) < 2) {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
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
        case 'a':  // anti-aliased lines
            opt_line_algo = 2;
            break;
        case 'n':  // number of lines
            if (sscanf(optarg, "%d", &opt_num_lines) != 1 || opt_num_lines < 1 || opt_num_lines > MAX_LINES) {
                fprintf(stderr, "Invalid number of lines '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 's':  // skip min,max points
            if (sscanf(optarg, "%d,%d", &opt_skip_min, &opt_skip_max) < 2 || opt_skip_min < 1 || opt_skip_max < opt_skip_min) {
                fprintf(stderr, "Invalid skip values '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        default:
            return usage(argv[0]);
        }
    }

    // retrieve arg text
    const char *text = argv[optind];
    if (text && strncmp(text, "one", 3) == 0) {
        opt_draw_num = 1;
    }
    else if (text && strncmp(text, "two", 3) == 0) {
        opt_draw_num = 2;
    }
    else if (text && strncmp(text, "four", 4) == 0) {
        opt_draw_num = 4;
    }
    else {
        fprintf(stderr, "Missing 'one', 'two', or 'four'\n");
        return usage(argv[0]);
    }

    return 0;
}

// ------------------------------------------------------------------------------------------

// random int in range min to max inclusive
int randomInt(int min, int max) {
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

    switch (opt_line_algo) {
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
  if (lines_idx >= opt_num_lines) { lines_idx = 0; }

  if (reset) {
    // randomly position a new line
    lines[lines_idx].x1 = randomInt(1, opt_width - 2);
    lines[lines_idx].y1 = randomInt(1, opt_height - 2);
    lines[lines_idx].x2 = randomInt(1, opt_width - 2);
    lines[lines_idx].y2 = randomInt(1, opt_height - 2);

    // random skip values
    skip.x1 = (randomInt(0, 1)) ? randomInt(opt_skip_min, opt_skip_max) : -randomInt(opt_skip_min, opt_skip_max);
    skip.y1 = (randomInt(0, 1)) ? randomInt(opt_skip_min, opt_skip_max) : -randomInt(opt_skip_min, opt_skip_max);
    skip.x2 = (randomInt(0, 1)) ? randomInt(opt_skip_min, opt_skip_max) : -randomInt(opt_skip_min, opt_skip_max);
    skip.y2 = (randomInt(0, 1)) ? randomInt(opt_skip_min, opt_skip_max) : -randomInt(opt_skip_min, opt_skip_max);
  }
  else {
    // move the line
    lines[lines_idx].x1 = lines[old_idx].x1 + skip.x1;
    lines[lines_idx].y1 = lines[old_idx].y1 + skip.y1;
    lines[lines_idx].x2 = lines[old_idx].x2 + skip.x2;
    lines[lines_idx].y2 = lines[old_idx].y2 + skip.y2;

    // reverse direction of step values of points that hit border
    if (lines[lines_idx].x1 <= 0)          { skip.x1 = randomInt(opt_skip_min, opt_skip_max);      }
    if (lines[lines_idx].x1 >= opt_width)  { skip.x1 = randomInt(opt_skip_min, opt_skip_max) * -1; }
    if (lines[lines_idx].y1 <= 0)          { skip.y1 = randomInt(opt_skip_min, opt_skip_max);      }
    if (lines[lines_idx].y1 >= opt_height) { skip.y1 = randomInt(opt_skip_min, opt_skip_max) * -1; }
    if (lines[lines_idx].x2 <= 0)          { skip.x2 = randomInt(opt_skip_min, opt_skip_max);      }
    if (lines[lines_idx].x2 >= opt_width)  { skip.x2 = randomInt(opt_skip_min, opt_skip_max) * -1; }
    if (lines[lines_idx].y2 <= 0)          { skip.y2 = randomInt(opt_skip_min, opt_skip_max);      }
    if (lines[lines_idx].y2 >= opt_height) { skip.y2 = randomInt(opt_skip_min, opt_skip_max) * -1; }
  }
  return lines[lines_idx];
}

Line lastLine() {
  // global: lines_idx, lines[]
  int last_idx = lines_idx + 1;
  if (last_idx >= opt_num_lines) { last_idx = 0; }
  return lines[last_idx];
}

void drawAllLines(const Line &line, const Color &color, UDPFlaschenTaschen &canvas) {

    drawLine( line.x1, line.y1, line.x2, line.y2, color, canvas);
    if (opt_draw_num >= 2) {
        drawLine( opt_width - line.x1, opt_height - line.y1, 
                  opt_width - line.x2, opt_height - line.y2, color, canvas);
    }
    if (opt_draw_num == 4) {
        drawLine( opt_width - line.x1, line.y1, opt_width - line.x2, line.y2, color, canvas);
        drawLine( line.x1, opt_height - line.y1, line.x2, opt_height - line.y2, color, canvas);
    }
}

// ------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // seed the random generator
    srandom(time(NULL)); 

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // init vars
    lines_idx = 0;
    Color transparent = Color(0, 0, 0);
    Color color = nextColor(TRUE);
    Line line = nextLine(TRUE);
    int count = 0;
    time_t starttime = time(NULL);

    do {
        // erase last line
        drawAllLines(lastLine(), transparent, canvas);

        // draw colored line
        color = nextColor(FALSE);
        line = nextLine(FALSE);
        drawAllLines(line, color, canvas);

        // send canvas
        canvas.SetOffset(0, 0, opt_layer);
        canvas.Send();
        usleep(opt_delay * 1000);

        count++;
        if (count == INT_MAX) { count=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received );

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    return 0;
}
