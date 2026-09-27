CXX ?= g++
PKG_CONFIG ?= pkg-config
CXXFLAGS += -std=c++17 -O2 -Wall -Wextra $(shell $(PKG_CONFIG) --cflags gtkmm-3.0 gtk-layer-shell-0 libpulse)
LDLIBS += $(shell $(PKG_CONFIG) --libs gtkmm-3.0 gtk-layer-shell-0 libpulse)

.PHONY: all clean

all: visualizer-light visualizer-dark

visualizer-light: visualizer.cpp
	$(CXX) $(CXXFLAGS) $< $(LDLIBS) -o $@

visualizer-dark: visualizer-dark.cpp
	$(CXX) $(CXXFLAGS) $< $(LDLIBS) -o $@

clean:
	rm -f visualizer-light visualizer-dark
