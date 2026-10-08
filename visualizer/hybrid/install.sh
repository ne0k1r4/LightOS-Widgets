#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

make -C "$DIR"

mkdir -p "$HOME/.local/bin"
mkdir -p "$HOME/.config/Light/bin"

install -m 755 \
  "$DIR/lightos-hybrid-visualizer" \
  "$HOME/.local/bin/lightos-hybrid-visualizer"

for mode in light dark; do
    cat > "$HOME/.config/Light/bin/visualizer-$mode" <<'SH'
#!/usr/bin/env bash
exec "$HOME/.local/bin/lightos-hybrid-visualizer" "$@"
SH
    chmod +x "$HOME/.config/Light/bin/visualizer-$mode"
done
