#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEST="${XDG_CONFIG_HOME:-$HOME/.config}/quickshell/lightos-death-note-v11"

command -v quickshell >/dev/null || {
    echo "Quickshell is required." >&2
    exit 1
}

test -f "$ROOT/shell.qml" || {
    echo "LightOS pill source missing." >&2
    exit 1
}

mkdir -p "$DEST"

cp -a "$ROOT/." "$DEST/"

mkdir -p "$HOME/.local/bin"

cat > "$HOME/.local/bin/lightos-pill-v11" <<'LAUNCHER'
#!/usr/bin/env bash
set -euo pipefail

PILL="$HOME/.config/quickshell/lightos-death-note-v11/shell.qml"

case "${1:-start}" in
    start)
        if ! pgrep -af '[q]uickshell.*lightos-death-note-v11' >/dev/null; then
            quickshell -p "$PILL" &
        fi
        ;;
    stop)
        pkill -f '[q]uickshell.*lightos-death-note-v11' || true
        ;;
    toggle)
        if pgrep -af '[q]uickshell.*lightos-death-note-v11' >/dev/null; then
            pkill -f '[q]uickshell.*lightos-death-note-v11'
        else
            quickshell -p "$PILL" &
        fi
        ;;
esac
LAUNCHER

chmod +x "$HOME/.local/bin/lightos-pill-v11"

echo "LightOS Death Note Gothic V11.1 installed."
