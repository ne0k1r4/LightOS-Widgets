# LightOS Widgets

Clock widget and audio visualizer for the LightOS desktop. Gothic anime themed, wallpaper-color driven.

## Install

```sh
git clone https://github.com/ne0k1r4/LightOS-Widgets.git
cd LightOS-Widgets
./install.sh
```

Installs to `~/.config/Light/widgets/`. Started automatically by Hyprland on login.

### Dependencies

```sh
sudo pacman -S gtkmm3 gtk-layer-shell libpulse jsoncpp
```

## Widgets

### Clock Widget
Displays gothic anime artwork that changes each hour (12 images for hours 1–12).
Images are stored in `~/.config/Light/assets/clocks/clock1.png` through `clock12.png`.

Started with:
```sh
~/.config/Light/widgets/clock_widget
```

### Audio Visualizer
Wave+fill style spectrum analyzer. Two variants:
- `visualizer` — Light theme colors
- `visualizer-dark` — Dark theme colors

Colors are driven by the active wallpaper palette from `~/.config/waybar/wallpaper-colors.css`.

## License

MIT — see [LICENSE](LICENSE). Original MIT notices for imported source files retained in `LICENSES/`.
