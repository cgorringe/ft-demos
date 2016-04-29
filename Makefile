CXXFLAGS=-Wall -O3 -I../common -I.
OBJECTS=udp-flaschen-taschen.o

MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

FFMPEG_LDFLAGS=`pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`

ALL=simple-example simple-animation random-dots quilt black plasma

all : $(ALL)

simple-example: simple-example.cc $(OBJECTS)
simple-animation: simple-animation.cc $(OBJECTS)
random-dots: random-dots.cc $(OBJECTS)
quilt: quilt.cc $(OBJECTS)
black: black.cc $(OBJECTS)
plasma: plasma.cc $(OBJECTS)

% : %.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(ALL) $(OBJECTS)
