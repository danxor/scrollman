#include <err.h>
#include <stdlib.h>

#include <X11/Xutil.h>

#include "config.h"
#include "layout.h"
#include "scrollman.h"
#include "x11.h"

static struct workspace *
workspace_new(void)
{
	struct workspace	*ws;

	if ((ws = calloc(1, sizeof(*ws))) == NULL)
		err(1, "calloc");
	TAILQ_INIT(&ws->columns);
	TAILQ_INIT(&ws->floats);
	TAILQ_INSERT_TAIL(&wm.workspaces, ws, entry);
	return (ws);
}

static int
workspace_empty(struct workspace *ws)
{
	return (TAILQ_EMPTY(&ws->columns) && TAILQ_EMPTY(&ws->floats));
}

/*
 * Keep workspaces dynamic, like Niri: there is always one empty
 * workspace at the bottom, and no other empty workspace except the
 * one being shown.
 */
static void
workspaces_tidy(void)
{
	struct workspace	*ws, *next;

	TAILQ_FOREACH_SAFE(ws, &wm.workspaces, entry, next) {
		if (workspace_empty(ws) && ws != wm.ws && next != NULL) {
			TAILQ_REMOVE(&wm.workspaces, ws, entry);
			free(ws);
		}
	}
	if (!workspace_empty(TAILQ_LAST(&wm.workspaces, workspace_q)))
		workspace_new();
}

void
layout_init(void)
{
	TAILQ_INIT(&wm.workspaces);
	wm.ws = workspace_new();
}

/* Workspace n, counting from 1, or the bottom one if there are fewer. */
struct workspace *
workspace_nth(int n)
{
	struct workspace	*ws;

	TAILQ_FOREACH(ws, &wm.workspaces, entry)
		if (--n == 0)
			return (ws);
	return (TAILQ_LAST(&wm.workspaces, workspace_q));
}

static struct workspace *
client_ws(struct client *c)
{
	return (c->col != NULL ? c->col->ws : c->ws);
}

struct client *
client_find(Window win)
{
	struct workspace	*ws;
	struct column		*col;
	struct client		*c;

	TAILQ_FOREACH(ws, &wm.workspaces, entry) {
		TAILQ_FOREACH(col, &ws->columns, entry)
			TAILQ_FOREACH(c, &col->clients, entry)
				if (c->win == win)
					return (c);
		TAILQ_FOREACH(c, &ws->floats, entry)
			if (c->win == win)
				return (c);
	}
	return (NULL);
}

struct client *
selected(void)
{
	if (wm.ws->float_focus != NULL)
		return (wm.ws->float_focus);
	if (wm.ws->focus == NULL)
		return (NULL);
	return (wm.ws->focus->focus);
}

static struct column *
column_new(struct workspace *ws)
{
	struct column	*col;

	if ((col = calloc(1, sizeof(*col))) == NULL)
		err(1, "calloc");
	TAILQ_INIT(&col->clients);
	col->ws = ws;
	col->width = wm.sw * config.column_percent / 100;
	return (col);
}

/* Add a column right of the workspace's focused column, and focus it. */
static void
column_insert(struct workspace *ws, struct column *col)
{
	col->ws = ws;
	if (ws->focus != NULL)
		TAILQ_INSERT_AFTER(&ws->columns, ws->focus, col, entry);
	else
		TAILQ_INSERT_TAIL(&ws->columns, col, entry);
	ws->focus = col;
}

/* Take a column out of its workspace, moving focus to a neighbour. */
static void
column_remove(struct column *col)
{
	struct workspace	*ws = col->ws;
	struct column		*ncol;

	if ((ncol = TAILQ_PREV(col, column_q, entry)) == NULL)
		ncol = TAILQ_NEXT(col, entry);
	TAILQ_REMOVE(&ws->columns, col, entry);
	if (ws->focus == col)
		ws->focus = ncol;
}

/*
 * Take a window out of its column, and drop the column if it is now
 * empty. A window leaving its column also leaves fullscreen.
 */
static void
detach(struct client *c)
{
	struct column	*col = c->col;
	struct client	*next;

	if (c->fullscreen) {
		c->fullscreen = 0;
		col->width = col->fs_width;
		x_set_fullscreen(c->win, 0);
	}

	if ((next = TAILQ_NEXT(c, entry)) == NULL)
		next = TAILQ_PREV(c, client_q, entry);
	TAILQ_REMOVE(&col->clients, c, entry);

	if (TAILQ_EMPTY(&col->clients)) {
		column_remove(col);
		free(col);
	} else if (col->focus == c)
		col->focus = next;
}

/* Dialogs, utility windows, splash screens and fixed-size windows float. */
static int
should_float(Window win, Window *parent)
{
	XSizeHints	 hints;
	long		 supplied;

	*parent = None;
	if (XGetTransientForHint(wm.dpy, win, parent) && *parent != None)
		return (1);
	if (x_has_atom(win, wm.atom[NET_WM_WINDOW_TYPE],
	    wm.atom[NET_WM_WINDOW_TYPE_DIALOG]) ||
	    x_has_atom(win, wm.atom[NET_WM_WINDOW_TYPE],
	    wm.atom[NET_WM_WINDOW_TYPE_UTILITY]) ||
	    x_has_atom(win, wm.atom[NET_WM_WINDOW_TYPE],
	    wm.atom[NET_WM_WINDOW_TYPE_SPLASH]))
		return (1);
	if (XGetWMNormalHints(wm.dpy, win, &hints, &supplied) &&
	    (hints.flags & PMinSize) && (hints.flags & PMaxSize) &&
	    hints.min_width == hints.max_width &&
	    hints.min_height == hints.max_height)
		return (1);
	return (0);
}

/*
 * Float a window at the size it asks for, within its size hints,
 * centred over its parent on the parent's workspace.
 */
static void
manage_float(struct client *c, Window parent, XWindowAttributes *wa)
{
	struct client	*p = client_find(parent);
	XSizeHints	 hints;
	long		 supplied;
	int		 px = 0, py = 0, pw = wm.sw, ph = wm.sh;
	int		 bw = config.border_width;

	c->parent = parent;
	c->ws = p != NULL ? client_ws(p) : wm.ws;
	c->w = wa->width;
	c->h = wa->height;
	if (XGetWMNormalHints(wm.dpy, c->win, &hints, &supplied)) {
		if ((hints.flags & PMinSize) && c->w < hints.min_width)
			c->w = hints.min_width;
		if ((hints.flags & PMinSize) && c->h < hints.min_height)
			c->h = hints.min_height;
		if ((hints.flags & PMaxSize) && c->w > hints.max_width)
			c->w = hints.max_width;
		if ((hints.flags & PMaxSize) && c->h > hints.max_height)
			c->h = hints.max_height;
	}

	/* A parent on a hidden workspace is parked off-screen. */
	if (p != NULL && c->ws == wm.ws) {
		px = p->x;
		py = p->y;
		pw = p->w;
		ph = p->h;
	}
	c->x = px + (pw - c->w) / 2 - bw;
	c->y = py + (ph - c->h) / 2 - bw;
	if (c->x < 0)
		c->x = 0;
	if (c->y < 0)
		c->y = 0;

	TAILQ_INSERT_TAIL(&c->ws->floats, c, entry);
}

/* Put a new window in its own column, right of the focused one, or float it. */
void
manage(Window win)
{
	XWindowAttributes	 wa;
	struct column		*col;
	struct client		*c;
	Window			 parent;

	if (!XGetWindowAttributes(wm.dpy, win, &wa) || wa.override_redirect)
		return;
	if (client_find(win) != NULL)
		return;

	if ((c = calloc(1, sizeof(*c))) == NULL)
		err(1, "calloc");
	c->win = win;

	if (should_float(win, &parent))
		manage_float(c, parent, &wa);
	else {
		col = column_new(wm.ws);
		c->col = col;
		TAILQ_INSERT_TAIL(&col->clients, c, entry);
		col->focus = c;
		column_insert(wm.ws, col);
	}
	workspaces_tidy();

	XSetWindowBorderWidth(wm.dpy, win, config.border_width);
	/* Hold every click until we have focused the window; see events.c. */
	XGrabButton(wm.dpy, AnyButton, AnyModifier, win, False,
	    ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	x_set_wm_state(win, NormalState);
	if (config.focus_follows_mouse)
		XSelectInput(wm.dpy, win, EnterWindowMask);
	XMapWindow(wm.dpy, win);

	if (client_ws(c) == wm.ws)
		focus(c);
	else
		arrange();

	/* Some programs ask for fullscreen before they are mapped. */
	if (x_has_atom(win, wm.atom[NET_WM_STATE],
	    wm.atom[NET_WM_STATE_FULLSCREEN]))
		set_fullscreen(c, 1);
}

void
unmanage(struct client *c)
{
	struct client	*parent = NULL;

	if (c->col == NULL) {
		TAILQ_REMOVE(&c->ws->floats, c, entry);
		if (c->ws->float_focus == c) {
			c->ws->float_focus = NULL;
			parent = client_find(c->parent);
			if (parent != NULL && client_ws(parent) != wm.ws)
				parent = NULL;
		}
	} else
		detach(c);
	x_set_wm_state(c->win, WithdrawnState);
	free(c);

	workspaces_tidy();
	focus(parent != NULL ? parent : selected());
}

/*
 * A window alone in its column joins the neighbouring column on the
 * given side. A window sharing its column leaves it for a new column
 * on that side.
 */
void
consume_or_expel(struct client *c, int right)
{
	struct column	*col = c->col;
	struct column	*dst;

	if (TAILQ_NEXT(c, entry) == NULL &&
	    TAILQ_PREV(c, client_q, entry) == NULL) {
		if (right)
			dst = TAILQ_NEXT(col, entry);
		else
			dst = TAILQ_PREV(col, column_q, entry);
		if (dst == NULL)
			return;
	} else {
		dst = column_new(col->ws);
		if (right)
			TAILQ_INSERT_AFTER(&col->ws->columns, col, dst, entry);
		else
			TAILQ_INSERT_BEFORE(col, dst, entry);
	}

	detach(c);
	c->col = dst;
	TAILQ_INSERT_TAIL(&dst->clients, c, entry);
	focus(c);
}

void
workspace_focus(struct workspace *ws)
{
	wm.ws = ws;
	workspaces_tidy();
	focus(selected());
}

/* Move a column to another workspace and follow it there. */
void
column_to_workspace(struct column *col, struct workspace *dst)
{
	if (col->ws == dst)
		return;
	column_remove(col);
	column_insert(dst, col);
	workspace_focus(dst);
}

/*
 * Move a window to another workspace, staying here. A tiled window
 * gets its own column there.
 */
void
client_to_workspace(struct client *c, struct workspace *dst)
{
	struct column	*col;

	if (client_ws(c) == dst)
		return;

	if (c->col == NULL) {
		TAILQ_REMOVE(&c->ws->floats, c, entry);
		if (c->ws->float_focus == c)
			c->ws->float_focus = NULL;
		c->ws = dst;
		TAILQ_INSERT_TAIL(&dst->floats, c, entry);
	} else {
		detach(c);
		col = column_new(dst);
		c->col = col;
		TAILQ_INSERT_TAIL(&col->clients, c, entry);
		col->focus = c;
		column_insert(dst, col);
	}
	workspaces_tidy();
	focus(selected());
}

/* A fullscreen window in a column makes the column full width meanwhile. */
void
set_fullscreen(struct client *c, int on)
{
	if (c->fullscreen == on)
		return;
	c->fullscreen = on;
	if (c->col != NULL) {
		if (on) {
			c->col->fs_width = c->col->width;
			c->col->width = wm.sw;
		} else
			c->col->width = c->col->fs_width;
	}
	x_set_fullscreen(c->win, on);

	if (c == selected())
		focus(c);
	else
		arrange();
}

/* Show and focus a window that asked for it, switching workspace if needed. */
void
activate(struct client *c)
{
	struct workspace	*ws = client_ws(c);

	if (ws != wm.ws) {
		wm.ws = ws;
		workspaces_tidy();
	}
	focus(c);
}

/* Scroll just enough to show the whole column, but not past the strip. */
static void
scroll_to(struct column *col)
{
	struct column	*it;
	int		 x = 0, total = 0;

	TAILQ_FOREACH(it, &wm.ws->columns, entry) {
		if (it == col)
			x = total;
		total += it->width;
	}

	if (x < wm.ws->scroll)
		wm.ws->scroll = x;
	else if (x + col->width > wm.ws->scroll + wm.sw)
		wm.ws->scroll = x + col->width - wm.sw;

	if (wm.ws->scroll > total - wm.sw)
		wm.ws->scroll = total - wm.sw;
	if (wm.ws->scroll < 0)
		wm.ws->scroll = 0;
}

void
focus(struct client *c)
{
	if (c == NULL) {
		wm.ws->scroll = 0;
		arrange();
		XSetInputFocus(wm.dpy, wm.root, RevertToPointerRoot,
		    CurrentTime);
		x_set_active(None);
		return;
	}

	if (c->col == NULL)
		wm.ws->float_focus = c;
	else {
		wm.ws->float_focus = NULL;
		c->col->focus = c;
		wm.ws->focus = c->col;
		scroll_to(c->col);
	}
	arrange();
	XSetInputFocus(wm.dpy, c->win, RevertToPointerRoot, CurrentTime);
	x_set_active(c->win);
}

static void
arrange_workspace(struct workspace *ws, int yoff)
{
	struct column	*col;
	struct client	*c, *sel = NULL;
	int		 x, y, h, n, i, bw;

	if (ws == wm.ws)
		sel = selected();

	x = -ws->scroll;
	TAILQ_FOREACH(col, &ws->columns, entry) {
		n = 0;
		TAILQ_FOREACH(c, &col->clients, entry)
			n++;

		y = 0;
		i = 0;
		TAILQ_FOREACH(c, &col->clients, entry) {
			h = (wm.sh - y) / (n - i);
			bw = c->fullscreen ? 0 : config.border_width;
			/*
			 * Park columns that are fully off-screen just outside
			 * it, so X's 16-bit coordinates never overflow.
			 */
			if (x + col->width <= 0)
				c->x = -col->width;
			else if (x >= wm.sw)
				c->x = wm.sw;
			else
				c->x = x;
			c->y = yoff + (c->fullscreen ? 0 : y);
			c->w = col->width - 2 * bw;
			c->h = (c->fullscreen ? wm.sh : h) - 2 * bw;
			XSetWindowBorderWidth(wm.dpy, c->win, bw);
			XMoveResizeWindow(wm.dpy, c->win, c->x, c->y,
			    c->w, c->h);
			XSetWindowBorder(wm.dpy, c->win,
			    c == sel ? wm.border_focus : wm.border_normal);
			/* Cover the column's other windows. */
			if (c->fullscreen)
				XRaiseWindow(wm.dpy, c->win);
			y += h;
			i++;
		}
		x += col->width;
	}

	/* Floating windows keep their own place, above the columns. */
	TAILQ_FOREACH(c, &ws->floats, entry) {
		if (c->fullscreen) {
			XSetWindowBorderWidth(wm.dpy, c->win, 0);
			XMoveResizeWindow(wm.dpy, c->win, 0, yoff,
			    wm.sw, wm.sh);
		} else {
			XSetWindowBorderWidth(wm.dpy, c->win,
			    config.border_width);
			XMoveResizeWindow(wm.dpy, c->win, c->x,
			    yoff == 0 ? c->y : yoff, c->w, c->h);
		}
		XSetWindowBorder(wm.dpy, c->win,
		    c == sel ? wm.border_focus : wm.border_normal);
		XRaiseWindow(wm.dpy, c->win);
	}
	if (sel != NULL && sel->col == NULL)
		XRaiseWindow(wm.dpy, sel->win);
}

/* Show the current workspace, and park the others just below the screen. */
void
arrange(void)
{
	struct workspace	*ws;
	XEvent			 ev;

	TAILQ_FOREACH(ws, &wm.workspaces, entry)
		arrange_workspace(ws, ws == wm.ws ? 0 : wm.sh);

	/* Windows moving under a still pointer must not take the focus. */
	if (config.focus_follows_mouse) {
		XSync(wm.dpy, False);
		while (XCheckMaskEvent(wm.dpy, EnterWindowMask, &ev))
			;
	}
}
