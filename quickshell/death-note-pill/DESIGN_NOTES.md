# V11 design notes
- Compact pill target: 320 x 44 px; expanded pill target: 720 x 44 px.
- Top placement: 12 px, centered horizontally.
- Capsule radius: 22 px, with child artwork inset from the rounded ends and capsule clipping enabled.
- Preserve Misa/cross art and the existing transparent ornament/icon asset library from V10.
- Maintain V10 Python system integration and popover components; no Hyprland or Waybar edits.
- Runtime rendering and system integration must be verified on the target machine.


## V11.1 polish
- Shinigami Awakening: 550 ms expansion, 350 ms collapse, coordinated reveal.
- One-shot silver border glint, dormant while idle.
- Popover opening no longer mutates pinned state.
- Runtime validation still required on Arch/Hyprland.
