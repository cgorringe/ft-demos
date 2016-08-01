FLASCHEN_TASCHEN_API_DIR=ft/api

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

ALL=simple-example simple-animation random-dots quilt black plasma1 plasma2 plasma nb-logo blur lines hack fractal midi kbd2midi words life

all : $(ALL)

simple-example: simple-example.cc
simple-animation: simple-animation.cc
random-dots: random-dots.cc
quilt: quilt.cc
black: black.cc
plasma1: plasma1.cc
plasma2: plasma2.cc
plasma: plasma.cc
nb-logo: nb-logo.cc
blur: blur.cc
lines: lines.cc
hack: hack.cc
fractal: fractal.cc
midi: midi.cc
kbd2midi: kbd2midi.cc
words: words.cc
life: life.cc

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(FTLIB):
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f $(ALL)
