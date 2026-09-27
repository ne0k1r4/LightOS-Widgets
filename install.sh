#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEST="${XDG_BIN_HOME:-$HOME/.local/bin}"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
command -v make >/dev/null || { echo 'make is required.' >&2; exit 1; }
command -v pkg-config >/dev/null || { echo 'pkg-config is required.' >&2; exit 1; }
pkg-config --exists gtkmm-3.0 gtk-layer-shell-0 || { echo 'GTKmm 3 and gtk-layer-shell development files are required.' >&2; exit 1; }
pkg-config --exists gtk+-3.0 gtk-layer-shell-0 jsoncpp gio-2.0 libcurl || { echo 'GTK 3, gtk-layer-shell, jsoncpp, gio, and libcurl development files are required.' >&2; exit 1; }
pkg-config --exists libpulse || { echo 'PulseAudio development files are required.' >&2; exit 1; }
[[ -f "$CONFIG_HOME/Light/assets/clocks/clock1.png" ]] || { echo 'Clock artwork is missing. Install LightOS desktop defaults first.' >&2; exit 1; }
[[ -f "$CONFIG_HOME/Light/assets/settings/background-mem.png" ]] || { echo 'Visualizer artwork is missing. Install LightOS Assets first.' >&2; exit 1; }
make -C "$ROOT"
install -d "$DEST" "$CONFIG_HOME/Light/bin" "$CONFIG_HOME/Light/widgets/visualizer/light" "$CONFIG_HOME/Light/widgets/visualizer/dark"
install -m755 "$ROOT/scripts/lightos-widget" "$DEST/lightos-widget"
install -m755 "$ROOT/widgets/light-widget-daemon" "$CONFIG_HOME/Light/bin/light-widget-daemon"
install -m755 "$ROOT/widgets/light-widget-client" "$CONFIG_HOME/Light/bin/light-widget-client"
install -m755 "$ROOT/widgets/light-widget-daemon" "$DEST/light-widget-daemon"
install -m755 "$ROOT/widgets/light-widget-client" "$DEST/light-widget-client"
install -m755 "$ROOT/clock/clock_widget" "$CONFIG_HOME/Light/widgets/clock_widget"
install -m755 "$ROOT/visualizer/visualizer-light" "$CONFIG_HOME/Light/widgets/visualizer/light/visualizer"
install -m755 "$ROOT/visualizer/visualizer-dark" "$CONFIG_HOME/Light/widgets/visualizer/dark/visualizer"
printf 'Built and installed LightOS widget suite, clock, and visualizers.\n'
