#! /usr/bin/env python

import numpy as np
import flaschen_np
import fsa
import time
import argparse

def main():
    parser = argparse.ArgumentParser(description='Draw Sierpinski triangles',
                                     formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog)
    )

    # Optimization
    parser.add_argument('--host', type=str, default='ft.noise', help='Host to send packets to')
    parser.add_argument('--port', type=int, default=1337, help='Port to send packets to')
    parser.add_argument('--height', type=int, default=35, help='Canvas height')
    parser.add_argument('--width', type=int, default=45, help='Canvas width')
    parser.add_argument('--layer', '-l', type=int, default=11, help='Canvas layer (0-15)')

    parser.add_argument('--time', '-t', type=float, default=30, help='How long to run for before exiting')
    parser.add_argument('--sleep', '-s', type=float, default=.03, help='How long to sleep after each line')
    parser.add_argument('--embed', '-e', action='store_true', help='Drop into IPython shell after finishing')
    parser.add_argument('--clear', '-x', action='store_true', help='Clear at end')

    parser.add_argument('--color', '-c', type=str, default='blacktransp',
                        choices=('debug', 'randblack', 'blacktransp'),
                        help='Which color pattern to use (debug: green on blue. randblack: random on black. blacktrans: black on transparent)')
    parser.add_argument('--pattern', '-p', type=str, default='sierp',
                        choices=('sierp', '30'),
                        help='Which FSA rule to use')
    
    args = parser.parse_args()
    
    ff = flaschen_np.FlaschenNP(args.host, args.port, args.width, args.height, args.layer)
    line0 = np.zeros(ff.data.shape[1], dtype='bool')
    line0[line0.shape[0]/2] = True

    if args.pattern == 'sierp':
        # Sierpinski
        patterns = [[False, False, True], [True, False, False]]
    elif args.pattern == '30':
        # Rule 30 chaos
        patterns = [[True, False, False], [False, True, True], [False, True, False], [False, False, True]]
    else:
        raise Exception('Unknown rule')

    if args.color == 'debug':
        color_0 = [80, 80, 160]
        color_1 = [1, 255, 1]
    elif args.color == 'randblack':
        color_0 = [1, 1, 1]
        color_1 = 'rand'
    elif args.color == 'blacktransp':
        color_0 = [0, 0, 0]
        color_1 = [1, 1, 1]
    else:
        raise Exception('Unknown color')

    fs = fsa.FlaschenFSA(ff, line0, patterns, color_0=color_0, color_1=color_1)

    time_start = time.time()
    while time.time() < time_start + args.time:
        fs.step()
        fs.send()
        time.sleep(args.sleep)

    if args.embed:
        import IPython
        IPython.embed()

    if args.clear:
        fs.ff.zero()
        fs.send()
        fs.send()

if __name__ == '__main__':
    main()
