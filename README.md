## Flaschen-Taschen Demos

By [Carl Gorringe](http://carl.gorringe.org)

Here are some programs that I wrote for the [Noisebridge Flaschen Taschen Project](https://noisebridge.net/wiki/Flaschen_Taschen), based on [code written by hzeller](https://github.com/hzeller/flaschen-taschen).

![](img/random-dots.jpg)
![](img/quilt.jpg)
![](img/nb-logo.jpg)
![](img/plasma_large.jpg)
![](img/plasma_small.jpg)
![](img/lines.jpg)
![](img/blur_wave.jpg)
![](img/blur_pong.jpg)
![](img/hack.jpg)
![](img/fractal1.jpg)
![](img/fractal2.jpg)
![](img/life.jpg)
![](img/maze.jpg)


### Install How-to

This works on a unix-like OS such as Linux or Mac OS X.

#### Check out the project
This project uses the API provided in the `flaschen-taschen` project. We use
it in a sub-module, so we have to check out with the `--recursive` option:

```
  git clone --recursive https://github.com/cgorringe/ft-demos
```

Either make & run the server locally, or use the remote servers (see original README). Set up your display using the environment variable:

```
$ export FT_DISPLAY=localhost or servername
```

Now make and run the demos:

```
$ cd ft-demos
$ make all
$ ./random-dots
```

Use the `-?` command-line option on any demo program to list it's options.


### Demos provided
1. random-dots
2. quilt
3. nb-logo - logo that traverses the border
4. plasma - multi-colored plasma
5. blur bolt or boxes - burred simulated sound waves or rectangles
6. lines - like an old-school screensaver
7. hack - rotating letters + blur effect
8. fractal - zooming Mandelbrot fractal
9. midi - converts a midi bytestream into a player piano demo of scrolling dots
10. life - Conway's Game of Life
11. maze - maze generator
12. sierpinski - Spierpinski's Triangle


### Noisebridge hosts
* ```ft.noise``` - Large [Flaschen-Taschen](https://noisebridge.net/wiki/Flaschen_Taschen) (45x35 bb)
* ```ftkleine.noise``` - Smaller Kleine (25x20 bb)
* ```bookcase.noise``` - [Library bookshelves](https://noisebridge.net/wiki/Bookshelves) (810x1 LEDs)
* ```square.noise``` - [Noise Square table](https://noisebridge.net/wiki/Noise_Square_Table)

_____

## License

Copyright (c) 2016 Carl Gorringe. All rights reserved.

Licensed under the the terms of the [GNU General Public License version 2 (GPLv2)](http://gnu.org/licenses/gpl-2.0.html).
