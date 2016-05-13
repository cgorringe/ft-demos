CXXFLAGS=-Wall -O3 -I../common -I.
OBJECTS=udp-flaschen-taschen.o

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

FFMPEG_LDFLAGS=`pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`

ALL=simple-example simple-animation random-dots quilt black plasma-old plasma nb-logo blur lines hack fractal midi kbd2midi

all : $(ALL)

simple-example: simple-example.cc $(OBJECTS)
simple-animation: simple-animation.cc $(OBJECTS)
random-dots: random-dots.cc $(OBJECTS)
quilt: quilt.cc $(OBJECTS)
black: black.cc $(OBJECTS)
plasma-old: plasma-old.cc $(OBJECTS)
plasma: plasma.cc $(OBJECTS)
nb-logo: nb-logo.cc $(OBJECTS)
blur: blur.cc $(OBJECTS)
lines: lines.cc $(OBJECTS)
hack: hack.cc $(OBJECTS)
fractal: fractal.cc $(OBJECTS)
midi: midi.cc $(OBJECTS)
kbd2midi: kbd2midi.cc $(OBJECTS)

% : %.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(ALL) $(OBJECTS)
