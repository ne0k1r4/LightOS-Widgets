CXX ?= g++
.PHONY: all clock visualizer widgets clean
all: clock visualizer widgets
clock:
	$(MAKE) -C clock
visualizer:
	$(MAKE) -C visualizer
widgets:
	$(MAKE) -C widgets
clean:
	$(MAKE) -C clock clean
	$(MAKE) -C visualizer clean
	$(MAKE) -C widgets clean
