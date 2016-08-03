FLASCHEN_TASCHEN_API_DIR=ft/api

CXXFLAGS=-Wall -O3 -I$(FLASCHEN_TASCHEN_API_DIR)/include -I.
LDFLAGS=-L$(FLASCHEN_TASCHEN_API_DIR)/lib -lftclient
FTLIB=$(FLASCHEN_TASCHEN_API_DIR)/lib/libftclient.a

ALL=simple-example simple-animation random-dots quilt black plasma1 plasma2 plasma nb-logo blur lines hack fractal midi kbd2midi words life

all : $(ALL)

% : %.cc $(FTLIB)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(FTLIB):
	make -C $(FLASCHEN_TASCHEN_API_DIR)/lib

clean:
	rm -f $(ALL)
