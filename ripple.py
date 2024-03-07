#! /usr/bin/env python

import numpy
import flaschen_np
import time
import argparse

def main():
    parser = argparse.ArgumentParser(description='Ripple',
                                     formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(prog))

    parser.add_argument('--host', type=str, default='localhost', help='Host to send packets to')
    parser.add_argument('--port', type=int, default=1337, help='Port to send packets to')
    parser.add_argument('--height', type=int, default=35, help='Canvas height')
    parser.add_argument('--width', type=int, default=45, help='Canvas width')
    parser.add_argument('--layer', '-l', type=int, default=11, help='Canvas layer (0-15)')
    parser.add_argument('--time', '-t', type=float, default=1, help='How long to run for before exiting')
    parser.add_argument('--sleep', '-s', type=float, default=.01, help='How long to sleep after each line')

    args = parser.parse_args()

    Lx = 10
    Nx = args.width
    Ly = 10
    Ny = args.height

    x_vec = numpy.linspace(0, Lx, Nx)
    dx = x_vec[2] - x_vec[1]

    y_vec = numpy.linspace(0, Ly, Ny)
    dy = y_vec[2] - y_vec[1] 

    dt = .025
    Nt = 2000

    c = 1 
    u = numpy.zeros([Nt, len(x_vec), len(y_vec)])

    u[0, Nx // 2, Ny // 2] = numpy.sin(0)
    u[1, Nx // 2, Ny // 2] = numpy.sin(1/10)

    for t in range(1, Nt-1):
        # print(t/Nt)
        for x in range(1, Nx-1):
            for y in range(1, Ny-1):
                if (t < 100):
                    u[t, Nx // 2, Ny // 2] = numpy.sin(t / 10)

                u[t+1, x, y] = c**2 * dt**2 * ( ((u[t, x+1, y] - 2*u[t, x, y] + u[t, x-1, y])/(dx**2)) + ((u[t, x, y+1] - 2*u[t, x, y] + u[t, x, y-1])/(dy**2)) ) + 2*u[t, x, y] - u[t-1, x, y]

    ff = flaschen_np.FlaschenNP(args.host, args.port, args.width, args.height, args.layer)

    for t in range(int(args.time)):
        for i in list(u) + list(u[::-1]):
            for x in range(0, Nx):
                for y in range(0, Ny):
                    ff.data[y][x][0] = int( ((i[x][y] + 1) * 127.5) / 10 )
                    ff.data[y][x][1] = int( ((i[x][y] + 1) * 127.5) / 2 )
                    ff.data[y][x][2] = int( ((i[x][y] + 1) * 127.5) )

            ff.send()
            time.sleep(args.sleep)


if __name__ == '__main__':
    main()
