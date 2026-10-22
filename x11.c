#include <err.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "config.h"
#include "layout.h"
#include "scrollman.h"
#include "x11.h"

/* In the order of the enum in scrollman.h. */
static char *atom_names[NATOMS] = {
	"WM_PROTOCOLS",
	"WM_DELETE_WINDOW",
	"WM_STATE",
	"UTF8_STRING",
	"_NET_SUPPORTED",
	"_NET_SUPPORTING_WM_CHECK",
	"_NET_WM_NAME",
	"_NET_ACTIVE_WINDOW",
	"_NET_WM_STATE",
	"_NET_WM_STATE_FULLSCREEN",
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DIALOG",
	"_NET_WM_WINDOW_TYPE_UTILITY",
	"_NET_WM_WINDOW_TYPE_SPLASH",
};

static int
xerror_start(Display *dpy, XErrorEvent *ee)
{
	errx(1, "another window manager is already running");
}

/*
 * Never exit on an X error. Windows can vanish at any moment, so
 * BadWindow is expected and not worth a warning. XGetErrorText() is
 * avoided because it reads a file, which pledge(2) does not allow.
 */
static int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow)
		return (0);
	warnx("X error %d on request %d", ee->error_code, ee->request_code);
	return (0);
}

/* A bad color from the config file falls back to the built-in one. */
static unsigned long
x_color(const char *name, const char *fallback)
{
	Colormap	 cmap = DefaultColormap(wm.dpy, wm.screen);
	XColor		 c;

	if (XParseColor(wm.dpy, cmap, name, &c) &&
	    XAllocColor(wm.dpy, cmap, &c))
		return (c.pixel);
	warnx("cannot allocate color %s, using %s", name, fallback);
	if (XParseColor(wm.dpy, cmap, fallback, &c) &&
	    XAllocColor(wm.dpy, cmap, &c))
		return (c.pixel);
	errx(1, "cannot allocate color %s", fallback);
}

void
x_setup(void)
{
	if ((wm.dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "cannot open display %s", XDisplayName(NULL));
	wm.screen = DefaultScreen(wm.dpy);
	wm.root = RootWindow(wm.dpy, wm.screen);
	wm.sw = DisplayWidth(wm.dpy, wm.screen);
	wm.sh = DisplayHeight(wm.dpy, wm.screen);

	/* Only one client may select SubstructureRedirect on the root. */
	XSetErrorHandler(xerror_start);
	XSelectInput(wm.dpy, wm.root,
	    SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(wm.dpy, False);
	XSetErrorHandler(xerror);

	XInternAtoms(wm.dpy, atom_names, NATOMS, False, wm.atom);

	/* Tell EWMH clients that a compliant window manager is running. */
	wm.check = XCreateSimpleWindow(wm.dpy, wm.root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(wm.dpy, wm.check, wm.atom[NET_SUPPORTING_WM_CHECK],
	    XA_WINDOW, 32, PropModeReplace, (unsigned char *)&wm.check, 1);
	XChangeProperty(wm.dpy, wm.check, wm.atom[NET_WM_NAME],
	    wm.atom[UTF8_STRING], 8, PropModeReplace,
	    (unsigned char *)"scrollman", 9);
	XChangeProperty(wm.dpy, wm.root, wm.atom[NET_SUPPORTING_WM_CHECK],
	    XA_WINDOW, 32, PropModeReplace, (unsigned char *)&wm.check, 1);
	XChangeProperty(wm.dpy, wm.root, wm.atom[NET_SUPPORTED], XA_ATOM, 32,
	    PropModeReplace, (unsigned char *)&wm.atom[NET_SUPPORTED],
	    NET_WM_WINDOW_TYPE_SPLASH - NET_SUPPORTED + 1);
	XDeleteProperty(wm.dpy, wm.root, wm.atom[NET_ACTIVE_WINDOW]);

	wm.border_focus = x_color(config.border_focus, DEFAULT_BORDER_FOCUS);
	wm.border_normal = x_color(config.border_normal, DEFAULT_BORDER_NORMAL);

	x_grab_keys();
}

/* Manage windows that were already open when scrollman started. */
void
x_scan(void)
{
	XWindowAttributes	 wa;
	Window			 root, parent, *wins;
	unsigned int		 i, n;

	if (!XQueryTree(wm.dpy, wm.root, &root, &parent, &wins, &n))
		return;
	for (i = 0; i < n; i++)
		if (XGetWindowAttributes(wm.dpy, wins[i], &wa) &&
		    !wa.override_redirect && wa.map_state == IsViewable)
			manage(wins[i]);
	if (wins != NULL)
		XFree(wins);
}

void
x_cleanup(void)
{
	XUngrabKey(wm.dpy, AnyKey, AnyModifier, wm.root);
	XSetInputFocus(wm.dpy, PointerRoot, RevertToPointerRoot,
	    CurrentTime);
	XDeleteProperty(wm.dpy, wm.root, wm.atom[NET_SUPPORTING_WM_CHECK]);
	XDestroyWindow(wm.dpy, wm.check);
	XCloseDisplay(wm.dpy);
}

/* Whether the atom list property prop of win contains value. */
int
x_has_atom(Window win, Atom prop, Atom value)
{
	Atom		 type, *atoms = NULL;
	unsigned long	 i, n, after;
	int		 format, found = 0;

	if (XGetWindowProperty(wm.dpy, win, prop, 0, 64, False, XA_ATOM,
	    &type, &format, &n, &after, (unsigned char **)&atoms) != Success)
		return (0);
	for (i = 0; i < n; i++)
		if (atoms[i] == value)
			found = 1;
	if (atoms != NULL)
		XFree(atoms);
	return (found);
}

void
x_set_fullscreen(Window win, int on)
{
	XChangeProperty(wm.dpy, win, wm.atom[NET_WM_STATE], XA_ATOM, 32,
	    PropModeReplace, (unsigned char *)&wm.atom[NET_WM_STATE_FULLSCREEN],
	    on ? 1 : 0);
}

void
x_set_active(Window win)
{
	if (win == None)
		XDeleteProperty(wm.dpy, wm.root, wm.atom[NET_ACTIVE_WINDOW]);
	else
		XChangeProperty(wm.dpy, wm.root, wm.atom[NET_ACTIVE_WINDOW],
		    XA_WINDOW, 32, PropModeReplace, (unsigned char *)&win, 1);
}

/* Set the ICCCM WM_STATE, e.g. NormalState or WithdrawnState. */
void
x_set_wm_state(Window win, long state)
{
	long	 data[2];

	data[0] = state;
	data[1] = None;
	XChangeProperty(wm.dpy, win, wm.atom[WM_STATE], wm.atom[WM_STATE], 32,
	    PropModeReplace, (unsigned char *)data, 2);
}

static void
update_numlock(void)
{
	XModifierKeymap	*mm;
	KeyCode		 numlock;
	int		 i, j;

	wm.numlockmask = 0;
	mm = XGetModifierMapping(wm.dpy);
	numlock = XKeysymToKeycode(wm.dpy, XK_Num_Lock);
	for (i = 0; i < 8; i++)
		for (j = 0; j < mm->max_keypermod; j++)
			if (mm->modifiermap[i * mm->max_keypermod + j] ==
			    numlock)
				wm.numlockmask = 1 << i;
	XFreeModifiermap(mm);
}

/* Grab every binding, also with Caps Lock and Num Lock on. */
void
x_grab_keys(void)
{
	unsigned int	 locks[4];
	KeyCode		 code;
	size_t		 i;
	int		 j;

	update_numlock();
	locks[0] = 0;
	locks[1] = LockMask;
	locks[2] = wm.numlockmask;
	locks[3] = wm.numlockmask | LockMask;

	XUngrabKey(wm.dpy, AnyKey, AnyModifier, wm.root);
	for (i = 0; i < nbindings; i++) {
		if ((code = XKeysymToKeycode(wm.dpy, bindings[i].keysym)) == 0)
			continue;
		for (j = 0; j < 4; j++)
			XGrabKey(wm.dpy, code, bindings[i].mod | locks[j],
			    wm.root, True, GrabModeAsync, GrabModeAsync);
	}
}

/* Drop Caps Lock, Num Lock and mouse buttons from a modifier state. */
unsigned int
x_clean_mask(unsigned int state)
{
	return (state & ~(LockMask | wm.numlockmask) &
	    (ShiftMask | ControlMask | Mod1Mask | Mod2Mask | Mod3Mask |
	    Mod4Mask | Mod5Mask));
}

/* Ask the window to close (ICCCM), or kill it if it does not support that. */
void
x_close(Window win)
{
	XEvent	 ev;
	Atom	*protos;
	int	 i, n, polite = 0;

	if (XGetWMProtocols(wm.dpy, win, &protos, &n)) {
		for (i = 0; i < n; i++)
			if (protos[i] == wm.atom[WM_DELETE_WINDOW])
				polite = 1;
		XFree(protos);
	}

	if (!polite) {
		XKillClient(wm.dpy, win);
		return;
	}

	ev.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = wm.atom[WM_PROTOCOLS];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = wm.atom[WM_DELETE_WINDOW];
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(wm.dpy, win, False, NoEventMask, &ev);
}
