// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// black
//
// Clears the Flaschen Taschen canvas.
// https://noisebridge.net/wiki/Flaschen_Taschen
//
// By default, connects to the installation at Noisebridge. If using a
// different display (e.g. a local terminal display)
// pass the hostname as parameter:
//
//  ./simple-example localhost
//
// .. or set the environment variable FT_DISPLAY to not worry about it
//
//  export FT_DISPLAY=localhost
//  ./simple-example

#include "udp-flaschen-taschen.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define DISPLAY_WIDTH  (9*5)  // large: 9*5
#define DISPLAY_HEIGHT (7*5)  //        7*5
#define Z_LAYER 10      // (0-15) 0=background

int main(int argc, char *argv[]) {
    const char *hostname = NULL;   // Will use default if not set otherwise.
    if (argc > 1) {
        hostname = argv[1];        // Hostname can be supplied as first arg
    }

    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket(hostname);
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    //canvas.Clear();
    while (1) {
        canvas.Fill(Color(1, 1, 1));

        // send canvas
        canvas.SetOffset(0, 0, Z_LAYER);
        canvas.Send();
        sleep(5);
    }
}
