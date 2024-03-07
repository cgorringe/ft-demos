#! /usr/bin/env python

import flaschen_np
import time
import argparse

def main():
    parser = argparse.ArgumentParser(description='Grid',
                                     formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog))

    parser.add_argument('--host', type=str, default='localhost', help='Host to send packets to')
    parser.add_argument('--port', type=int, default=1337, help='Port to send packets to')
    parser.add_argument('--height', type=int, default=35, help='Canvas height')
    parser.add_argument('--width', type=int, default=45, help='Canvas width')
    parser.add_argument('--layer', '-l', type=int, default=11, help='Canvas layer (0-15)')
    parser.add_argument('--time', '-t', type=float, default=30, help='How long to run for before exiting')
    parser.add_argument('--sleep', '-s', type=float, default=.005, help='How long to sleep after each line')
    
    args = parser.parse_args()
    
    ff = flaschen_np.FlaschenNP(args.host, args.port, args.width, args.height, args.layer)

    for t in range(int(args.time)):
        for i in list(range(255)) + list(range(255)[::-1]):
            for x in range(args.width):
                for y in range(args.height):
                    ff.data[y][x][0] = 255 - i if x%2==0 else 255 * (x/args.width)
                    ff.data[y][x][1] = 255 - i if y%2==0 else 255 * (y/args.height)
                    ff.data[y][x][2] = 255 * (y/args.height) if x%2==0 and y%2==0 else 255 * (x/args.width)

            ff.send()
            time.sleep(args.sleep)


if __name__ == '__main__':
    main()
