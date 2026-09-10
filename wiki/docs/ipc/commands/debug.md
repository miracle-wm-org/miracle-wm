# debug
Toggles the **debug overlay** — a transparent, always-on-top client that draws live
debugging information on top of every window. It is intended for diagnosing layout, clipping
and focus issues at runtime without reading the logs.

The overlay is the bundled `miracle-wm-debug-overlay` client (a GTK4 + `gtk4-layer-shell`
application). It is launched the first time you toggle it on and terminated when you toggle
it off.

## Syntax
```sh
debug [overlay] [toggle | on | off]
```

- `debug` / `debug overlay` / `debug overlay toggle` — toggle the overlay.
- `debug overlay on` (`show`, `enable`) — force the overlay on.
- `debug overlay off` (`hide`, `disable`) — force the overlay off.

## Example
```sh
# Toggle the overlay on/off
miraclemsg debug

# Equivalent, explicit forms
miraclemsg debug overlay
miraclemsg debug overlay on
miraclemsg debug overlay off
```

## What the overlay shows
- **Window geometry** — a solid outline at each window's logical position and size, with a
  colocated label (`#<id> <app_id>` and `x,y WxH`).
- **Clip / visible area** — a dashed outline showing the portion of the window that is
  actually drawn (its clip area), when it differs from the logical area.
- **Window under the cursor** — highlighted in yellow.
- **Cursor position** — a crosshair with an `x,y` readout.
- **Hidden windows** — listed in the HUD panel and (when on-screen) outlined in grey.
- **Copy to clipboard** — the HUD panel (top-right) has a "Copy debug state JSON" button and
  a per-window "copy" button that copy the relevant JSON to the system clipboard.

The overlay spans every monitor and is transparent to input everywhere **except** over its
HUD panel, so it never interferes with the windows beneath it.

## Configuration
The overlay client is configured through
[`wm_clients.debug_overlay`](../../configuration/wm_clients.md). Setting it to `"disabled"`
turns `miraclemsg debug` into a no-op; setting it to a custom executable name/path launches
that instead of the bundled overlay.

## Notes
- Introduced in v0.10.0.
- The overlay polls the compositor over IPC using
  [`GET_DEBUG_STATE`](../get_debug_state.md); you can request the same data directly for your
  own tooling.
