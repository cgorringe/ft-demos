#! /usr/bin/env python

import numpy
import flaschen_np
import time


def main():
    Lx = 10
    Nx = 35
    Ly = 10
    Ny = 35

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

    ff = flaschen_np.FlaschenNP('localhost', 1337, 35, 35, 11)
    for i in u:
        for x in range(0,35):
            for y in range(0,35):
                ff.data[x][y][0]=int( ((i[x][y] + 1) * 127.5) / 10 )
                ff.data[x][y][1]=int( ((i[x][y] + 1) * 127.5) / 2 )
                ff.data[x][y][2]=int( ((i[x][y] + 1) * 127.5) )

        ff.send()
        time.sleep(.01)


if __name__ == '__main__':
    main()
