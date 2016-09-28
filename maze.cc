// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// maze
// Copyright (c) 2016 Carl Gorringe (carl.gorringe.org)
// https://github.com/cgorringe/ft-demos
// 9/17/2016
//
// Maze Generator
//
// How to run:
//
// To see command line options:
//  ./maze -?
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./maze -h localhost
//
// or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./maze
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
#include <time.h>
#include <string>
#include <string.h>
#include <signal.h>
#include <stack>
// using namespace std;

// Defaults                      large  small
#define DISPLAY_WIDTH  (9*5)  //  9*5    5*5
#define DISPLAY_HEIGHT (7*5)  //  7*5    4*5
#define Z_LAYER 2      // (0-15) 0=background
#define DELAY 20

struct Position {
    Position() {}
    Position(int xx, int yy) : x(xx), y(yy) {}
    int x;
    int y;
};

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
bool opt_fgcolor = false, opt_bgcolor = false, opt_vcolor = false;
int opt_fg_R=0xFF, opt_fg_G=0xFF, opt_fg_B=0xFF;
int opt_vc_R=0, opt_vc_G=0, opt_vc_B=0;
int opt_bg_R=0, opt_bg_G=0, opt_bg_B=0;

int usage(const char *progname) {

    fprintf(stderr, "Maze (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 2)\n"
        "\t-t <timeout>   : Timeout exits after given seconds. (default 24hrs)\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 20)\n"
        "\t-c <RRGGBB>    : Maze color in hex (-c0 = transparent, default white)\n"
        "\t-v <RRGGBB>    : Visited color in hex (-v0 = transparent, default cycles)\n"
        "\t-b <RRGGBB>    : Background color in hex (-b0 = #010101, default transparent)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:c:v:b:")) != -1) {
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
        case 'c':  // initial maze color
            if (sscanf(optarg, "%02x%02x%02x", &opt_fg_R, &opt_fg_G, &opt_fg_B) != 3) {
                opt_fg_R=0, opt_fg_G=0, opt_fg_B=0;
                //fprintf(stderr, "Color parse error\n");
                //return usage(argv[0]);
            }
            opt_fgcolor = true;
            break;
        case 'v':  // visited maze color
            if (sscanf(optarg, "%02x%02x%02x", &opt_vc_R, &opt_vc_G, &opt_vc_B) != 3) {
                opt_vc_R=0, opt_vc_G=0, opt_vc_B=0;
                //fprintf(stderr, "Color parse error\n");
                //return usage(argv[0]);
            }
            opt_vcolor = true;
            break;
        case 'b':  // background color
            if (sscanf(optarg, "%02x%02x%02x", &opt_bg_R, &opt_bg_G, &opt_bg_B) != 3) {
                opt_bg_R=1, opt_bg_G=1, opt_bg_B=1;
            }
            opt_bgcolor = true;
            break;
        default:
            return usage(argv[0]);
        }
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

void clearArray( const int width, const int height, uint8_t some_array[] ) {

    for (int i=0; i < width * height; i++) { 
        some_array[i] = 0;
    }
}


/*
    Using the simple Depth-first search algorithm.

    1. Start at a random cell.
    2. Mark the current cell as visited, and get a list of its neighbors. 
       For each neighbor, starting with a randomly selected neighbor:
    3. If that neighbor hasn't been visited, remove the wall between this cell and that neighbor, 
       and then recurse with that neighbor as the current cell.

    NOTE: instead of calling a recursive function, I should create a stack to store pos_x & pos_y
    so that I can use the loop in main()
*/

int mazePos2PixelIndex(Position pos, int px_width) {
    return ((pos.y * 2 * px_width) + (pos.x * 2));
}

int wallIndexBetweenPositions(Position pos1, Position pos2, int px_width) {
    // returns pixel index of wall between two cell positions in maze
    // it's really the mid-point between the two cells
    return ( ((pos1.y + pos2.y) * px_width) + (pos1.x + pos2.x) );
}

void drawMaze(std::stack<Position> &cell_stack, int px_width, int px_height, uint8_t pixels[]) {

    if ( cell_stack.empty() ) { 
        //printf("cell_stack empty\n");  // DEBUG
        return;
    }

    int maze_width = (px_width / 2.0f) + 0.5;
    int maze_height = (px_height / 2.0f) + 0.5;
    Position pos = cell_stack.top();

    //int stack_size = cell_stack.size();
    //printf("top pos: (%d, %d); size: %d \n", pos.x, pos.y, stack_size);  // DEBUG

    // mark current pos as forground
    int cur_idx = mazePos2PixelIndex(pos, px_width);
    pixels[cur_idx] = 1;

    // create list of neighbors not yet visited, handling edge cases
    int n=0, temp_idx, wall_idx;
    Position neighbor[4];
    if (pos.y > 0) { 
        temp_idx = (((pos.y - 1) * 2 * px_width) + (pos.x * 2));
        if (pixels[temp_idx] == 0) { neighbor[n++] = Position(pos.x, pos.y - 1); }
    }
    if (pos.y < maze_height - 1) { 
        temp_idx = (((pos.y + 1) * 2 * px_width) + (pos.x * 2)); 
        if (pixels[temp_idx] == 0) { neighbor[n++] = Position(pos.x, pos.y + 1); }
    }
    if (pos.x > 0) { 
        temp_idx = ((pos.y * 2 * px_width) + ((pos.x - 1) * 2)); 
        if (pixels[temp_idx] == 0) { neighbor[n++] = Position(pos.x - 1, pos.y); }
    }
    if (pos.x < maze_width - 1) { 
        temp_idx = ((pos.y * 2 * px_width) + ((pos.x + 1) * 2)); 
        if (pixels[temp_idx] == 0) { neighbor[n++] = Position(pos.x + 1, pos.y); }
    }

    // pick a random neighbor
    if (n > 0) {
        int rand_idx = randomInt(0, n - 1);
        //printf("rand_idx: %d of %d\n", rand_idx, n);  // DEBUG

        // draw wall
        wall_idx = wallIndexBetweenPositions(pos, neighbor[rand_idx], px_width);
        pixels[wall_idx] = 1;

        cell_stack.push( neighbor[rand_idx] );

        //printf("neighbor[rand_idx] = (%d, %d)\n", neighbor[rand_idx].x, neighbor[rand_idx].y ); // DEBUG
        //printf("new size1: %lu \n", cell_stack.size() );  // DEBUG
    }
    else {
        // no neighbors left
        //printf("no neighbors\n");  // DEBUG

        // mark pos as visited then pop off stack
        pixels[cur_idx] = 2;
        cell_stack.pop();

        // draw visited wall
        if ( !cell_stack.empty() ) { 
            Position pos2 = cell_stack.top();
            wall_idx = wallIndexBetweenPositions(pos, pos2, px_width);
            pixels[wall_idx] = 2;
        }
    }

}

int main(int argc, char *argv[]) {

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    // seed the random generator
    srandom(time(NULL));

    // set the color palette to a rainbow of colors
    Color palette[256];
    colorGradient( 0,   31,  255, 0,   255, 0,   0,   255, palette );
    colorGradient( 32,  63,  0,   0,   255, 0,   255, 255, palette );
    colorGradient( 64,  95,  0,   255, 255, 0,   255,   0, palette );
    colorGradient( 96,  127, 0,   255, 0,   127, 255,   0, palette );
    colorGradient( 128, 159, 127, 255, 0,   255, 255,   0, palette );
    colorGradient( 160, 191, 255, 255, 0,   255, 127,   0, palette );
    colorGradient( 192, 223, 255, 127, 0,   255, 0,     0, palette );
    colorGradient( 224, 255, 255, 0,   0,   255, 0,   255, palette );

    // setup colors
    Color fg_color = Color(opt_fg_R, opt_fg_G, opt_fg_B);
    Color bg_color = Color(opt_bg_R, opt_bg_G, opt_bg_B);
    Color vc_color = Color(opt_vc_R, opt_vc_G, opt_vc_B);

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.Clear();

    // pixel buffer
    uint8_t pixels[opt_width * opt_height];
    clearArray(opt_width, opt_height, pixels);

    // setup maze
    int maze_width = opt_width / 2;
    int maze_height = opt_height / 2;
    uint8_t maze[maze_width * maze_height];
    clearArray(maze_width, maze_height, maze);
    std::stack<Position> cell_stack;

    // random initial position
    Position maze_pos = Position( randomInt(0, maze_width - 1), randomInt(0, maze_height - 1) );
    cell_stack.push(maze_pos);

    // TEST: draw initial maze position
    //int temp_idx = mazePos2PixelIndex(maze_pos, opt_width);
    //pixels[temp_idx] = 1;

    // handle break
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // other vars
    int count = 0, colr = 0;
    time_t starttime = time(NULL);
    //time_t respawn_time = starttime;

    do {
        //runGameOfLife(opt_width, opt_height, pixels);

        // TODO: call non-recursive stack-based maze function
        drawMaze(cell_stack, opt_width, opt_height, pixels);
        //printf("new size2: %lu \n", cell_stack.size() );  // DEBUG

        // set pixel color if cycling through palette
        if (!opt_vcolor) {
            vc_color = palette[colr];
        }

        // copy pixel buffer to canvas
        int dst = 0;
        for (int y=0; y < opt_height; y++) {
            for (int x=0; x < opt_width; x++) {
                canvas.SetPixel( x, y, ((pixels[dst] == 2) ? vc_color : ((pixels[dst] == 1) ? fg_color : bg_color)) );
                dst++;
            }
        }

        // send canvas
        canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
        canvas.Send();
        usleep(opt_delay * 1000);

        count++;
        if (count == INT_MAX) { count=0; }

        colr++;
        if (colr >= 256) { colr=0; }

    } while ( (difftime(time(NULL), starttime) <= opt_timeout) && !interrupt_received ); // && !cell_stack.empty()

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();

    if (interrupt_received) return 1;
    return 0;
}
