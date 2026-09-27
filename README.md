# LightOS Widgets

Source repository for the LightOS widget suite: the visualizer, clock, widget daemon and client, brightness, calendar, host, microphone, music, playerctl, recent-apps, stats, system-info, and volume widgets.

## Build

Run `make` to build the clock, light/dark visualizers, widget daemon, and client. `visualizer/build.sh` builds both visualizer variants.

## Install

Install the LightOS desktop defaults and their dependencies first, then run `./install.sh`. The installer places widget executables under `~/.config/Light` and helper commands under `~/.local/bin`. The main LightOS installer includes this repository as a component.

Build dependencies are GTKmm 3, GTK 3, gtk-layer-shell, PulseAudio, jsoncpp, gio, libcurl, pkg-config, and make.

The original MIT notices for source files imported from ElysiaOS are retained in [LICENSES/ElysiaOS-MIT.txt](LICENSES/ElysiaOS-MIT.txt). The repository's own license is in [LICENSE](LICENSE).
