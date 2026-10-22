# scrollman

A small scrolling window manager for OpenBSD, inspired by [Niri](https://github.com/YaLTeR/niri).

Windows are arranged in columns on an endless horizontal strip. Opening a
new window never shrinks the ones you already have; the strip just grows to
the right, and the view scrolls to keep the focused column on screen.
Workspaces are stacked vertically, one above the other.

scrollman is written in C99 against Xlib, has no dependencies beyond the
OpenBSD base system and xenocara, and runs under `pledge(2)`.

## Features

- **Scrollable columns** – each workspace is an infinite strip of columns;
  each column holds one or more windows stacked vertically.
- **Vertical workspaces** – created on demand, Niri-style, and reachable
  by number (1–10) or by moving up and down.
- **Column widths** – cycle through width presets, toggle full width, or
  shrink and grow in steps.
- **Consume and expel** – pull a window into the neighbouring column, or
  push it out into a column of its own.
- **Floating dialogs** – transient windows and dialog, utility and splash
  window types float above the columns.
- **Fullscreen** – by key binding or by EWMH request from the application.
- **Restart in place** – rebuild and restart without losing your windows.
- **Plain-text config** – `~/.scrollmanrc`, with sensible built-in defaults
  when it is missing.
- **Standards** – ICCCM essentials (`WM_NAME`, `WM_NORMAL_HINTS`,
  `WM_TRANSIENT_FOR`, `WM_DELETE_WINDOW`, `WM_STATE`) and a minimal EWMH
  subset (`_NET_SUPPORTED`, `_NET_WM_STATE_FULLSCREEN`,
  `_NET_WM_WINDOW_TYPE`, `_NET_ACTIVE_WINDOW`).

## Requirements

- OpenBSD with xenocara (the X sets installed)

Nothing else. No packages are needed.

## Building

```sh
make
doas make install
```

`make install` puts the binary in `/usr/local/bin`.

## Running

Add scrollman to your `~/.xsession` (used by xenodm) or `~/.xinitrc`:

```sh
exec scrollman
```

To try it out without replacing your current window manager, run it in a
nested X server:

```sh
Xephyr :1 -screen 1280x800 & sleep 1; DISPLAY=:1 ./scrollman
```

### Usage

```
scrollman [-c file]
```

| Option    | Description                                                |
|-----------|------------------------------------------------------------|
| `-c file` | Read configuration from `file` instead of `~/.scrollmanrc`. |

## Default key bindings

The default modifier is **Super** (the Windows/logo key). Bindings avoid
AltGr-only keys so they work on non-US layouts.

| Keys                    | Action                                    |
|-------------------------|-------------------------------------------|
| `Super+Return`            | Open a terminal (`xterm`)                 |
| `Super+Left` / `Right`    | Focus column to the left / right          |
| `Super+Up` / `Down`       | Focus window above / below                |
| `Ctrl+Super+Left` / `Right` | Move column left / right                |
| `Ctrl+Super+Up` / `Down`  | Move window up / down in its column       |
| `Super+,` / `Super+.`       | Consume or expel window left / right      |
| `Super+PageUp` / `PageDown` | Focus workspace above / below           |
| `Ctrl+Super+PageUp` / `PageDown` | Move column to workspace above / below |
| `Super+1` … `Super+0`       | Focus workspace 1 … 10                    |
| `Super+Shift+1` … `Super+Shift+0` | Move window to workspace 1 … 10     |
| `Super+R`                 | Cycle column width presets                |
| `Super+F`                 | Toggle full-width column                  |
| `Super+Shift+,` / `Super+Shift+.` | Shrink / grow column width          |
| `Super+Shift+F`           | Toggle fullscreen                         |
| `Super+Q`                 | Close window                              |
| `Super+Shift+R`           | Restart scrollman in place                |
| `Super+Shift+E`           | Quit                                      |

Clicking a window focuses it.

## Configuration

scrollman reads `~/.scrollmanrc` at startup. If the file is missing, the
built-in defaults are used. Errors are reported with line numbers and the
offending line is skipped.

```sh
# Appearance. Widths are % of the screen.
border-width		2
border-focus		#7fc8ff
border-normal		#505050
column-width		50
width-presets		33 50 67
width-step		10
focus-follows-mouse	no

# Key bindings: modifiers, a dash, then an X keysym name (see xev).
# Modifiers: M = Alt, 4 = Super, S = Shift, C = Control.
bind M-Return		spawn xterm
bind M-q		close
unbind 4-q
```

| Setting               | Default         | Description                                   |
|-----------------------|-----------------|-----------------------------------------------|
| `border-width`        | `1`             | Window border width in pixels                 |
| `border-focus`        | `#7fc8ff`       | Border colour of the focused window           |
| `border-normal`       | `#505050`       | Border colour of other windows                |
| `column-width`        | `50`            | Width of a new column, in % of the screen     |
| `width-presets`       | `33 50 67`      | Widths that `cycle-width` steps through (max 8) |
| `width-step`          | `10`            | Step used by `shrink-width` and `grow-width`  |
| `focus-follows-mouse` | `no`            | Focus the window under the pointer            |

The built-in bindings are always loaded first. `bind` adds or replaces a
binding, `unbind <key>` removes one, and `unbind all` removes them all.
Letters are case-insensitive, so `4-B` is the same as `4-b`; use `4S-b` for
Super+Shift+B.

See [`example.scrollmanrc`](example.scrollmanrc) for every setting and the
full list of default bindings and action names.

## Roadmap

Not yet supported:

- Multiple monitors
- Following screen size changes (RandR)
- Moving and resizing floating windows with the mouse
- Window rules
- Animations, a built-in bar, compositing

## License

ISC. See [`LICENSE`](LICENSE).
