#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEST="${XDG_BIN_HOME:-$HOME/.local/bin}"
mkdir -p "$DEST"
install -m755 "$ROOT/scripts/lightos-widget" "$DEST/lightos-widget"
printf 'Installed LightOS widget helper to %s\n' "$DEST/lightos-widget"
