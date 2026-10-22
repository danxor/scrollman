# Scrollman

A scrolling window manager for OpenBSD, inspired by Niri.

## Techstack

- **C**: The code is written in C99.
- **Xorg**: The display server is Xorg. Talk to it through Xlib.
- **No external dependencies**: Use only what ships with a plain OpenBSD install (base system and xenocara). No packages.
- **Build**: A plain `Makefile` that works with OpenBSD's BSD `make` and `cc` (clang). Headers in `/usr/X11R6/include`, libraries in `/usr/X11R6/lib`.

## Design

- **Columns**: Each workspace is a horizontal strip of columns with no fixed end. A column holds one or more windows stacked vertically. The view scrolls left and right to keep the focused column visible.
- **Workspaces**: Workspaces are stacked vertically, like Niri.
- **Config**: Read at runtime from `~/.scrollmanrc`. Fall back to built-in defaults if the file is missing.
- **Standards**: Support the ICCCM essentials (`WM_NAME`, `WM_NORMAL_HINTS`, `WM_TRANSIENT_FOR`, `WM_DELETE_WINDOW`) and a minimal EWMH subset (`_NET_SUPPORTED`, `_NET_WM_STATE_FULLSCREEN`, `_NET_WM_WINDOW_TYPE`, `_NET_ACTIVE_WINDOW`).
- **Out of scope for the first iteration**: multi-monitor support, animations, a built-in bar, compositing.

## Conventions

- **Ask questions**: If something is unclear or ambigous **DO NOT** start work, **ALWAYS** ask the questions before starting work
- **Minimal change**: Always try to keep the changes as small as possible to solve the problem the user wanted solved
- **Simplicity**: The code should be simple to read and easy to understand
- **Style**: Follow OpenBSD's `style(9)` (KNF).
- **Modules**: Split the code into modules by concern, each a `.c` file with a matching `.h`.
- **Errors**: Use `err(3)`/`errx(3)` for fatal errors and `warn(3)`/`warnx(3)` otherwise. Install an X error handler so a stale window never crashes the WM.
- **Security**: Use `pledge(2)` and `unveil(2)` where possible.
- **License**: ISC.
- **Version control**: No git for now.

## Development

- Code is edited on a Fedora host and built and run in an OpenBSD virtual machine.
- Claude has no access to the VM. The user copies the code over, builds, runs and tests it, and reports back compiler output and behaviour.
- Build: `make`
- Test inside the VM in a nested X server, without replacing the running WM:
  `Xephyr :1 -screen 1280x800 & sleep 1; DISPLAY=:1 ./scrollman`
  (the `sleep` gives Xephyr time to start listening)
- The VM's real session also runs scrollman, started by xenodm from `~/.xsession` as `/home/dopedog/scrollman/scrollman`. Alt+Shift+R restarts it in place after a rebuild; its stderr goes to `~/.xsession-errors`.
