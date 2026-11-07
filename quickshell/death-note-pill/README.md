# LightOS Death Note Gothic v11

V11 is a compact redesign built from the supplied V10 project. It keeps the existing Quickshell QML architecture and Python integration backend while changing the pill to a 320x44 compact / 720x44 expanded capsule.

## Included functionality inherited from V10
- Live clock and date
- Battery percentage and charging state from `/sys/class/power_supply`
- MPRIS metadata, album art, and playback controls via `playerctl`
- Wi-Fi state and toggle via NetworkManager (`nmcli`)
- Bluetooth power toggle via BlueZ (`bluetoothctl`)
- Volume status and control via PipeWire (`wpctl`)
- SwayNC unread count and optional notification history bridge
- Wallpaper enumeration from `~/Pictures/Wallpapers` and application through `awww`
- Calendar month navigation and wallpaper gallery

## Install safely
```bash
mkdir -p ~/.config/quickshell/lightos-death-note-v11
unzip LightOS_Death_Note_Gothic_v11.zip -d ~/.config/quickshell/lightos-death-note-v11
quickshell -p ~/.config/quickshell/lightos-death-note-v11
```

Stop the V10 instance before launching V11 to avoid overlapping panels. V10 is not modified by this package. No Hyprland autostart or Waybar configuration is changed.

## Notes and limitations
- QML runtime validation must be performed on the target Arch Linux / Hyprland / Quickshell installation; this build environment does not include Quickshell or the user's system services.
- The optional notification history bridge remains opt-in at `~/.cache/lightos-gothic-notifications.json`; without it, the UI truthfully reports the unread count and offers to open SwayNC.
- Brightness, Night Light, DND, and session controls are not added as nonfunctional visual buttons. V10 did not include corresponding backend actions, so these require confirming the user's preferred brightness/night-light/DND services before implementation.
- Artwork is inherited from V10 and resized with aspect preservation. The existing separate transparent Misa and cross/rose PNG assets are retained; no new portrait is represented as generated.

## Rollback
Simply stop the V11 Quickshell process and launch the previous installation. V11 uses its own directory and does not overwrite V10.

## Validation performed for this archive
- Python backend syntax checked with `python3 -m py_compile`.
- ZIP integrity checked after packaging.
- QML runtime, compositor rendering, MPRIS, NetworkManager, BlueZ, PipeWire, SwayNC and awww behavior are not claimed as tested in this non-Arch environment.


## V11.1 — Shinigami Awakening polish

- Expansion uses a 550 ms `OutCubic` transition; collapse uses 350 ms `InCubic`.
- Expanded controls fade and translate into place using a shared reveal progress value.
- A restrained one-shot silver glint runs only when the pill expands and stops on collapse.
- Opening a popover no longer silently changes the user's pinned state; the popover itself keeps the shell expanded.

### Verification notes

The QML source was updated and packaged. This environment does not have the user's Arch/Hyprland desktop or Quickshell runtime, so live rendering, pointer interaction, and QML runtime compatibility remain unverified. Test with `quickshell -p ~/.config/quickshell/lightos-death-note-v11`.
