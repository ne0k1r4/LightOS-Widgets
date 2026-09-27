CXX ?= g++
PKG_CONFIG ?= pkg-config
CXXFLAGS ?= -Wall -Wextra -std=c++17 -O2
GTK_FLAGS := $(shell $(PKG_CONFIG) --cflags --libs gtk+-3.0 gtk-layer-shell-0 jsoncpp gio-2.0 libcurl)
DAEMON_TARGET := light-widget-daemon
CLIENT_TARGET := light-widget-client
DAEMON_SOURCES := light-widget-daemon.cpp light-common.cpp playerctl-interface.cpp calendar-widget.cpp recent-apps-widget.cpp system-info-widget.cpp stats-widget.cpp music-widget.cpp music-daemon.cpp brightness-widget.cpp volume-widget.cpp mic-widget.cpp host-widget.cpp
CLIENT_SOURCES := music_widget.cpp
.PHONY: all clean install uninstall
all: $(DAEMON_TARGET) $(CLIENT_TARGET)
$(DAEMON_TARGET): $(DAEMON_SOURCES) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(DAEMON_SOURCES) $(GTK_FLAGS) -o $@
$(CLIENT_TARGET): $(CLIENT_SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@
clean:
	rm -f $(DAEMON_TARGET) $(CLIENT_TARGET)
install: all
	install -Dm755 $(DAEMON_TARGET) $(DESTDIR)$(HOME)/.local/bin/$(DAEMON_TARGET)
	install -Dm755 $(CLIENT_TARGET) $(DESTDIR)$(HOME)/.local/bin/$(CLIENT_TARGET)
uninstall:
	rm -f $(HOME)/.local/bin/$(DAEMON_TARGET) $(HOME)/.local/bin/$(CLIENT_TARGET)
