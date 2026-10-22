#ifndef SCROLLMAN_H
#define SCROLLMAN_H

#include <sys/queue.h>

#include <X11/Xlib.h>

struct column;
struct workspace;

/* A managed window. Floating windows have no column. */
struct client {
	TAILQ_ENTRY(client)	 entry;
	struct column		*col;
	struct workspace	*ws;		/* floating windows only */
	Window			 win;
	Window			 parent;	/* floating: WM_TRANSIENT_FOR */
	int			 x, y, w, h;	/* geometry set by arrange() */
	int			 fullscreen;
};
TAILQ_HEAD(client_q, client);

/* A column of windows stacked vertically. */
struct column {
	TAILQ_ENTRY(column)	 entry;
	struct workspace	*ws;
	struct client_q		 clients;
	struct client		*focus;
	int			 width;
	int			 prev_width;	/* width before full width, or 0 */
	int			 fs_width;	/* width before a window went fullscreen */
};
TAILQ_HEAD(column_q, column);

/* A horizontal strip of columns, seen through the screen. */
struct workspace {
	TAILQ_ENTRY(workspace)	 entry;
	struct column_q		 columns;
	struct column		*focus;
	struct client_q		 floats;	/* bottom to top */
	struct client		*float_focus;	/* NULL when a column has focus */
	int			 scroll;	/* strip x at the screen's left edge */
};
TAILQ_HEAD(workspace_q, workspace);

enum {
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_STATE,
	UTF8_STRING,
	/* From here to NET_WM_WINDOW_TYPE_SPLASH goes in _NET_SUPPORTED. */
	NET_SUPPORTED,
	NET_SUPPORTING_WM_CHECK,
	NET_WM_NAME,
	NET_ACTIVE_WINDOW,
	NET_WM_STATE,
	NET_WM_STATE_FULLSCREEN,
	NET_WM_WINDOW_TYPE,
	NET_WM_WINDOW_TYPE_DIALOG,
	NET_WM_WINDOW_TYPE_UTILITY,
	NET_WM_WINDOW_TYPE_SPLASH,
	NATOMS
};

struct wm {
	Display			*dpy;
	Window			 root;
	int			 screen;
	int			 sw, sh;
	unsigned int		 numlockmask;
	unsigned long		 border_focus;
	unsigned long		 border_normal;
	Atom			 atom[NATOMS];
	Window			 check;		/* _NET_SUPPORTING_WM_CHECK */
	struct workspace_q	 workspaces;	/* top to bottom */
	struct workspace	*ws;		/* the one shown */
	int			 running;
	int			 restart;	/* exec ourselves again on exit */
};

extern struct wm wm;

#endif
