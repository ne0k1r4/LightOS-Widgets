CXX ?= g++
PKG_CONFIG ?= pkg-config
CXXFLAGS += -std=c++17 -O2 -Wall -Wextra $(shell $(PKG_CONFIG) --cflags gtkmm-3.0 gtk-layer-shell-0)
LDLIBS += $(shell $(PKG_CONFIG) --libs gtkmm-3.0 gtk-layer-shell-0)
.PHONY: all clean
all: chrono-fate-test
chrono-fate-test: clock.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)
clean:
	rm -f chrono-fate-test
