CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
PKGS = gtkmm-3.0 gtk-layer-shell-0 libpulse-simple libpulse

all: lightos-hybrid-visualizer

lightos-hybrid-visualizer: hybrid.cpp
	$(CXX) $(CXXFLAGS) hybrid.cpp -o $@ \
	$(shell pkg-config --cflags --libs $(PKGS))

clean:
	rm -f lightos-hybrid-visualizer

.PHONY: all clean
