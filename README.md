# Miracle - a Wayland Compositor based on Mir

## Building
```
mkdir build
cd build
cmake ..
```

### Protocols
```
wayland-scanner client-header < ./src/protocols/xdg-shell.xml > ./src/protocols/xdg_shell.h
wayland-scanner private-code < ./src/protocols/xdg-shell.xml > ./src/protocols/xdg_shell.c
```

## Design
- Floating window management with tiling zones
- Integrated widgets for sound, networking, trays, etc.
- Task bar at the bottom
- CSS-driven design palette (GTK might have support here)
- Application opening and closing transitions

## Roadmap
- 0.1: Task bar
  - Show black bar on bottom of the screen
  - Show icons of open programs in the task bar
  - Highlight selected program
  - Be able to move programs around
  - Click to minimize program + click again to maximize program
  - Show black square for menu
  - List applications within the black square
- 0.2: CSS integration with task bar
- 0.3: Top bar 101 with clock widget
