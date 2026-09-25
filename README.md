# LightOS Widgets

Waybar helper modules for LightOS. This repository provides a small JSON-producing status command and ready-to-copy Waybar module examples.

## Install

```sh
./install.sh
```

The command is installed to `~/.local/bin/lightos-widget`. Available modules are CPU, memory, root storage, battery, and temperature. Sensors and battery data are shown when matching Linux interfaces are available.

Requirements: Bash, `awk`, `sed`; `jq` is used by the sample Waybar modules.

MIT licensed. See [LICENSE](LICENSE).
