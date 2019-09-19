// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
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

// ** NOT DONE **
// TODO: need to modify bdf-font.h & .cc code to draw fonts to
// a 1-byte per indexed pixel buffer, which I don't have time right now...


#include "udp-flaschen-taschen.h"
#include "bdf-font.h"
#include "config.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <time.h>
#include <math.h>

// Defaults
#define Z_LAYER 4      // (0-15) 0=background
#define DELAY 100
#define PALETTE_MAX 3  // 1=Nebula, 2=Fire, 3=Bluegreen
#define BLUR_DROP 32  // 8, 16
#define TEXT_MAX 200  // not used?
#define FONT_FILE "./fonts/5x5.bdf"


volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// ------------------------------------------------------------------------------------------
// Command Line Options

// option vars
const char *opt_hostname = NULL;
int opt_layer  = Z_LAYER;
double opt_time = 10;  // default time calculated if not given
int opt_width  = DISPLAY_WIDTH;
int opt_height = DISPLAY_HEIGHT;
int opt_xoff=0, opt_yoff=0;
int opt_delay   = DELAY;
int opt_palette = 1;  // default nebula
char opt_display_text[TEXT_MAX];  // not used?
ft::Font opt_font;

int usage(const char *progname) {

    fprintf(stderr, "Words (c) 2016 Carl Gorringe (carl.gorringe.org)\n");
    fprintf(stderr, "Usage: %s [options] <text>...\n", progname);
    fprintf(stderr, "Options:\n"
        "\t-g <W>x<H>[+<X>+<Y>] : Output geometry. (default 45x35+0+0)\n"
        "\t-l <layer>     : Layer 0-15. (default 1)\n"
        "\t-t <time>      : Total time in seconds to display all the words.\n"
        "\t-h <host>      : Flaschen-Taschen display hostname. (FT_DISPLAY)\n"
        "\t-d <delay>     : Delay between frames in milliseconds. (default 25)\n"
        "\t-p <palette>   : Set color palette to: (default 1)\n"
        "\t                  1=Nebula, 2=Fire, 3=Bluegreen\n"
        "\t-f <fontfile>  : Path to *.bdf font file. (default: fonts/5x5.bdf)\n"
    //    "\t-r <repeat>    : Repeat phrase x number of times, then exits. (default 1)\n"
    );
    return 1;
}

int cmdLine(int argc, char *argv[]) {

    // command line options
    int opt;
    while ((opt = getopt(argc, argv, "?g:l:t:h:d:p:f:")) != -1) {
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
        case 't':  // time
            if (sscanf(optarg, "%lf", &opt_time) != 1 || opt_time < 0) {
                fprintf(stderr, "Invalid time in seconds '%s'\n", optarg);
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
        case 'f':  // font
            if (!opt_font.LoadFont(optarg)) {
                fprintf(stderr, "Couldn't load font '%s'\n", optarg);
            }
            break;
        default:
            return usage(argv[0]);
        }
    }

    if (opt_font.height() < 0) {
        // use FONT_FILE if exists
        if (!opt_font.LoadFont(FONT_FILE)) {
            fprintf(stderr, "Couldn't load font '%s'\n", FONT_FILE);
            return usage(argv[0]);
        }
    }

    if (opt_height < 0) {
        opt_height = opt_font.height();
    }

    if (opt_width < 1 || opt_height < 1) {
        fprintf(stderr, "%dx%d is a rather unusual size\n", opt_width, opt_height);
        return usage(argv[0]);
    }

    // assign default DISPLAY_TEXT
    //strncpy(opt_display_text, DISPLAY_TEXT, TEXT_LENGTH);

    // retrieve arg text from remaining arguments
    /*
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
    //*/

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

// --------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    fprintf(stderr, "** NOT DONE YET **\n");

    // parse command line
    if (int e = cmdLine(argc, argv)) { return e; }

    srandom(time(NULL)); // seed the random generator
/*
    int scroll_delay_ms = 50;
    bool run_forever = true;
    Color fg(0xff, 0xff, 0xff);
    Color bg(0, 0, 0);
    int r, g, b;
//*/

    // Assemble all non-option arguments to one text.
    std::string str;
    for (int i = optind; i < argc; ++i) {
        str.append(argv[i]).append(" ");
    }
    const char *text = str.c_str();

    // Eat leading spaces and figure out if we end up with any
    // text at all.
    while (isspace(*text)) {
        ++text;
    }
    if (!*text) {
        fprintf(stderr, "This looks like a very empty text.\n");
        return 1;
    }

    // open socket and create our canvas
    const int socket = OpenFlaschenTaschenSocket(opt_hostname);
    UDPFlaschenTaschen canvas(socket, opt_width, opt_height);
    canvas.SetOffset(opt_xoff, opt_yoff, opt_layer);
    canvas.Clear();

    // ***** TODO *****

/*
    // Center in in the available display space.
    const int y_pos = (height - font.height()) / 2 + font.baseline();

    // dry-run to determine total number of pixels.
    const int total_len = DrawText(&display, font, 0, y_pos, fg, NULL, text);
//*/
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

/*
    if (scroll_delay_ms > 0) {
        do {
            display.Fill(bg);
            for (int s = 0; s < total_len + width && !interrupt_received; ++s) {
                DrawText(&display, font, width - s, y_pos, fg, &bg, text);
                display.Send();
                usleep(scroll_delay_ms * 1000);
            }
        } while (run_forever && !interrupt_received);
    }
    else {
        // No scrolling, just show directly and once.
        display.Fill(bg);
        DrawText(&display, font, 0, y_pos, fg, &bg, text);
        display.Send();
    }

    // Don't let leftovers cover up content unless we set a no-scrolling
    // text explicitly
    if (off_z > 0 && scroll_delay_ms > 0) {
        display.Clear();
        display.Send();
    }
//*/

    // clear canvas on exit
    canvas.Clear();
    canvas.Send();
    close(socket);

    if (interrupt_received) return 1;
    return 0;
}
