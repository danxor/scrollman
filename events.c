#include "config.h"
#include "events.h"
#include "layout.h"
#include "scrollman.h"
#include "x11.h"

/* Tell a window its geometry, as ICCCM requires after a refused request. */
static void
send_configure(struct client *c)
{
	XConfigureEvent	 ce;

	ce.type = ConfigureNotify;
	ce.display = wm.dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->fullscreen ? 0 : config.border_width;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(wm.dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

static void
on_configure_request(XConfigureRequestEvent *ev)
{
	XWindowChanges	 wc;
	struct client	*c;

	/*
	 * Floating windows may move and resize themselves. The layout
	 * decides where all other managed windows go.
	 */
	if ((c = client_find(ev->window)) != NULL) {
		if (c->col == NULL && !c->fullscreen) {
			if (ev->value_mask & CWX)
				c->x = ev->x;
			if (ev->value_mask & CWY)
				c->y = ev->y;
			if (ev->value_mask & CWWidth)
				c->w = ev->width;
			if (ev->value_mask & CWHeight)
				c->h = ev->height;
			arrange();
		} else
			send_configure(c);
		return;
	}

	wc.x = ev->x;
	wc.y = ev->y;
	wc.width = ev->width;
	wc.height = ev->height;
	wc.border_width = ev->border_width;
	wc.sibling = ev->above;
	wc.stack_mode = ev->detail;
	XConfigureWindow(wm.dpy, ev->window, ev->value_mask, &wc);
}

static void
on_key_press(XKeyEvent *ev)
{
	KeySym		 sym = XLookupKeysym(ev, 0);
	unsigned int	 state = x_clean_mask(ev->state);
	size_t		 i;

	for (i = 0; i < nbindings; i++)
		if (bindings[i].keysym == sym &&
		    x_clean_mask(bindings[i].mod) == state)
			bindings[i].fn(bindings[i].arg);
}

/* Focus the clicked window, then pass the click on to it. */
static void
on_button_press(XButtonEvent *ev)
{
	struct client	*c;

	if ((c = client_find(ev->window)) != NULL && c != selected())
		focus(c);
	XAllowEvents(wm.dpy, ReplayPointer, CurrentTime);
}

/* Focus follows mouse; only windows selected for EnterWindowMask get here. */
static void
on_enter(XCrossingEvent *ev)
{
	struct client	*c;

	if (ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return;
	if ((c = client_find(ev->window)) != NULL && c != selected())
		focus(c);
}

/* EWMH requests: fullscreen on or off, and focusing a window. */
static void
on_client_message(XClientMessageEvent *ev)
{
	struct client	*c;
	Atom		 fs = wm.atom[NET_WM_STATE_FULLSCREEN];
	long		 action;

	if ((c = client_find(ev->window)) == NULL)
		return;

	if (ev->message_type == wm.atom[NET_WM_STATE]) {
		if ((Atom)ev->data.l[1] != fs && (Atom)ev->data.l[2] != fs)
			return;
		/* 0 is remove, 1 is add, 2 is toggle. */
		action = ev->data.l[0];
		set_fullscreen(c, action == 1 || (action == 2 && !c->fullscreen));
	} else if (ev->message_type == wm.atom[NET_ACTIVE_WINDOW])
		activate(c);
}

static void
on_gone(Window win)
{
	struct client	*c;

	if ((c = client_find(win)) != NULL)
		unmanage(c);
}

void
event_loop(void)
{
	XEvent	 ev;

	wm.running = 1;
	while (wm.running && XNextEvent(wm.dpy, &ev) == 0) {
		switch (ev.type) {
		case MapRequest:
			manage(ev.xmaprequest.window);
			break;
		case ConfigureRequest:
			on_configure_request(&ev.xconfigurerequest);
			break;
		case UnmapNotify:
			on_gone(ev.xunmap.window);
			break;
		case DestroyNotify:
			on_gone(ev.xdestroywindow.window);
			break;
		case KeyPress:
			on_key_press(&ev.xkey);
			break;
		case ButtonPress:
			on_button_press(&ev.xbutton);
			break;
		case ClientMessage:
			on_client_message(&ev.xclient);
			break;
		case EnterNotify:
			on_enter(&ev.xcrossing);
			break;
		case MappingNotify:
			XRefreshKeyboardMapping(&ev.xmapping);
			if (ev.xmapping.request == MappingKeyboard)
				x_grab_keys();
			break;
		}
	}
}
