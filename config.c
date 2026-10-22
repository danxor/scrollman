#include <errno.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "actions.h"
#include "config.h"

#define MODKEY	Mod4Mask

struct config config = {
	.border_width = 1,
	.border_focus = DEFAULT_BORDER_FOCUS,
	.border_normal = DEFAULT_BORDER_NORMAL,
	.column_percent = 50,
	.width_step = 10,
	.width_presets = { 33, 50, 67 },
	.nwidth_presets = 3,
	.focus_follows_mouse = 0,
};

static const struct binding default_bindings[] = {
	{ MODKEY,		XK_Return,	act_spawn,		"xterm" },
	{ MODKEY,		XK_Left,	act_focus_left,		NULL },
	{ MODKEY,		XK_Right,	act_focus_right,	NULL },
	{ MODKEY,		XK_Up,		act_focus_up,		NULL },
	{ MODKEY,		XK_Down,	act_focus_down,		NULL },
	{ MODKEY | ControlMask,	XK_Left,	act_move_column_left,	NULL },
	{ MODKEY | ControlMask,	XK_Right,	act_move_column_right,	NULL },
	{ MODKEY | ControlMask,	XK_Up,		act_move_window_up,	NULL },
	{ MODKEY | ControlMask,	XK_Down,	act_move_window_down,	NULL },
	{ MODKEY,		XK_comma,	act_consume_or_expel_left, NULL },
	{ MODKEY,		XK_period,	act_consume_or_expel_right, NULL },
	{ MODKEY,		XK_Prior,	act_focus_workspace_up,	NULL },
	{ MODKEY,		XK_Next,	act_focus_workspace_down, NULL },
	{ MODKEY | ControlMask,	XK_Prior,	act_move_column_to_workspace_up, NULL },
	{ MODKEY | ControlMask,	XK_Next,	act_move_column_to_workspace_down, NULL },
	{ MODKEY,		XK_1,		act_focus_workspace,	"1" },
	{ MODKEY,		XK_2,		act_focus_workspace,	"2" },
	{ MODKEY,		XK_3,		act_focus_workspace,	"3" },
	{ MODKEY,		XK_4,		act_focus_workspace,	"4" },
	{ MODKEY,		XK_5,		act_focus_workspace,	"5" },
	{ MODKEY,		XK_6,		act_focus_workspace,	"6" },
	{ MODKEY,		XK_7,		act_focus_workspace,	"7" },
	{ MODKEY,		XK_8,		act_focus_workspace,	"8" },
	{ MODKEY,		XK_9,		act_focus_workspace,	"9" },
	{ MODKEY,		XK_0,		act_focus_workspace,	"10" },
	{ MODKEY | ShiftMask,	XK_1,		act_move_window_to_workspace, "1" },
	{ MODKEY | ShiftMask,	XK_2,		act_move_window_to_workspace, "2" },
	{ MODKEY | ShiftMask,	XK_3,		act_move_window_to_workspace, "3" },
	{ MODKEY | ShiftMask,	XK_4,		act_move_window_to_workspace, "4" },
	{ MODKEY | ShiftMask,	XK_5,		act_move_window_to_workspace, "5" },
	{ MODKEY | ShiftMask,	XK_6,		act_move_window_to_workspace, "6" },
	{ MODKEY | ShiftMask,	XK_7,		act_move_window_to_workspace, "7" },
	{ MODKEY | ShiftMask,	XK_8,		act_move_window_to_workspace, "8" },
	{ MODKEY | ShiftMask,	XK_9,		act_move_window_to_workspace, "9" },
	{ MODKEY | ShiftMask,	XK_0,		act_move_window_to_workspace, "10" },
	{ MODKEY,		XK_r,		act_cycle_width,	NULL },
	{ MODKEY,		XK_f,		act_full_width,		NULL },
	{ MODKEY | ShiftMask,	XK_f,		act_fullscreen,		NULL },
	/* Shift+, and Shift+. are ; and : on a Swedish layout. */
	{ MODKEY | ShiftMask,	XK_comma,	act_shrink_width,	NULL },
	{ MODKEY | ShiftMask,	XK_period,	act_grow_width,		NULL },
	{ MODKEY,		XK_q,		act_close,		NULL },
	{ MODKEY | ShiftMask,	XK_e,		act_quit,		NULL },
	{ MODKEY | ShiftMask,	XK_r,		act_restart,		NULL },
};

struct binding	*bindings;
size_t		 nbindings;

enum { ARG_NONE, ARG_COMMAND, ARG_WORKSPACE };

/* The names used for actions in the config file. */
static const struct {
	const char	*name;
	void		(*fn)(const char *);
	int		 arg;
} actions[] = {
	{ "spawn",			act_spawn,		ARG_COMMAND },
	{ "focus-left",			act_focus_left,		ARG_NONE },
	{ "focus-right",		act_focus_right,	ARG_NONE },
	{ "focus-up",			act_focus_up,		ARG_NONE },
	{ "focus-down",			act_focus_down,		ARG_NONE },
	{ "move-column-left",		act_move_column_left,	ARG_NONE },
	{ "move-column-right",		act_move_column_right,	ARG_NONE },
	{ "move-window-up",		act_move_window_up,	ARG_NONE },
	{ "move-window-down",		act_move_window_down,	ARG_NONE },
	{ "consume-or-expel-left",	act_consume_or_expel_left, ARG_NONE },
	{ "consume-or-expel-right",	act_consume_or_expel_right, ARG_NONE },
	{ "focus-workspace-up",		act_focus_workspace_up,	ARG_NONE },
	{ "focus-workspace-down",	act_focus_workspace_down, ARG_NONE },
	{ "move-column-to-workspace-up", act_move_column_to_workspace_up, ARG_NONE },
	{ "move-column-to-workspace-down", act_move_column_to_workspace_down, ARG_NONE },
	{ "focus-workspace",		act_focus_workspace,	ARG_WORKSPACE },
	{ "move-window-to-workspace",	act_move_window_to_workspace, ARG_WORKSPACE },
	{ "cycle-width",		act_cycle_width,	ARG_NONE },
	{ "full-width",			act_full_width,		ARG_NONE },
	{ "shrink-width",		act_shrink_width,	ARG_NONE },
	{ "grow-width",			act_grow_width,		ARG_NONE },
	{ "fullscreen",			act_fullscreen,		ARG_NONE },
	{ "close",			act_close,		ARG_NONE },
	{ "quit",			act_quit,		ARG_NONE },
	{ "restart",			act_restart,		ARG_NONE },
};

/* Where we are in the file, for error messages. */
static const char	*cur_path;
static int		 cur_line;

static void
bad(const char *fmt, ...)
{
	va_list	 ap;
	char	 msg[256];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	warnx("%s:%d: %s", cur_path, cur_line, msg);
}

/* Split off the next blank-separated word, or return NULL at the end. */
static char *
next_word(char **p)
{
	char	*w;

	*p += strspn(*p, " \t");
	if (**p == '\0')
		return (NULL);
	w = *p;
	*p += strcspn(*p, " \t");
	if (**p != '\0')
		*(*p)++ = '\0';
	return (w);
}

static int
no_more_words(char **p)
{
	if (next_word(p) == NULL)
		return (1);
	bad("too many arguments");
	return (0);
}

static void
binding_set(unsigned int mod, KeySym sym, void (*fn)(const char *),
    const char *arg)
{
	size_t	 i;

	for (i = 0; i < nbindings; i++)
		if (bindings[i].mod == mod && bindings[i].keysym == sym)
			break;
	if (i == nbindings) {
		bindings = reallocarray(bindings, nbindings + 1,
		    sizeof(*bindings));
		if (bindings == NULL)
			err(1, "reallocarray");
		nbindings++;
	}
	bindings[i].mod = mod;
	bindings[i].keysym = sym;
	bindings[i].fn = fn;
	bindings[i].arg = arg;
}

static void
binding_remove(unsigned int mod, KeySym sym)
{
	size_t	 i;

	for (i = 0; i < nbindings; i++) {
		if (bindings[i].mod == mod && bindings[i].keysym == sym) {
			bindings[i] = bindings[--nbindings];
			return;
		}
	}
}

/* Parse a key like MS-comma: modifiers, a dash, then a keysym name. */
static int
parse_key(char *s, unsigned int *mod, KeySym *sym)
{
	KeySym	 upper;
	char	*dash, *p;

	*mod = 0;
	if ((dash = strchr(s, '-')) != NULL) {
		for (p = s; p < dash; p++) {
			switch (*p) {
			case 'C':
				*mod |= ControlMask;
				break;
			case 'M':
				*mod |= Mod1Mask;
				break;
			case '4':
				*mod |= Mod4Mask;
				break;
			case 'S':
				*mod |= ShiftMask;
				break;
			default:
				bad("unknown modifier \"%c\" in \"%s\"", *p, s);
				return (0);
			}
		}
		s = dash + 1;
	}
	if ((*sym = XStringToKeysym(s)) == NoSymbol) {
		bad("unknown key \"%s\"", s);
		return (0);
	}
	/*
	 * Key presses are matched by their unshifted, lowercase symbol,
	 * so M-B means M-b. Shift is written as S, as in MS-b.
	 */
	XConvertCase(*sym, sym, &upper);
	return (1);
}

static void
parse_bind(char **p)
{
	unsigned int	 mod;
	KeySym		 sym;
	const char	*errstr;
	char		*key, *name, *arg = NULL;
	size_t		 i;

	if ((key = next_word(p)) == NULL || (name = next_word(p)) == NULL) {
		bad("usage: bind key action [argument]");
		return;
	}
	if (!parse_key(key, &mod, &sym))
		return;
	for (i = 0; i < sizeof(actions) / sizeof(actions[0]); i++)
		if (strcmp(actions[i].name, name) == 0)
			break;
	if (i == sizeof(actions) / sizeof(actions[0])) {
		bad("unknown action \"%s\"", name);
		return;
	}

	switch (actions[i].arg) {
	case ARG_NONE:
		if (!no_more_words(p))
			return;
		break;
	case ARG_COMMAND:
		/* The command is the rest of the line. */
		*p += strspn(*p, " \t");
		if (**p == '\0') {
			bad("%s needs a command", name);
			return;
		}
		arg = *p;
		break;
	case ARG_WORKSPACE:
		if ((arg = next_word(p)) == NULL) {
			bad("%s needs a workspace number", name);
			return;
		}
		strtonum(arg, 1, 10, &errstr);
		if (errstr != NULL) {
			bad("workspace \"%s\" is %s", arg, errstr);
			return;
		}
		if (!no_more_words(p))
			return;
		break;
	}

	if (arg != NULL && (arg = strdup(arg)) == NULL)
		err(1, "strdup");
	binding_set(mod, sym, actions[i].fn, arg);
}

static void
parse_unbind(char **p)
{
	unsigned int	 mod;
	KeySym		 sym;
	char		*key;

	if ((key = next_word(p)) == NULL) {
		bad("usage: unbind key | all");
		return;
	}
	if (!no_more_words(p))
		return;
	if (strcmp(key, "all") == 0)
		nbindings = 0;
	else if (parse_key(key, &mod, &sym))
		binding_remove(mod, sym);
}

static void
parse_number(char **p, int min, int max, int *out)
{
	const char	*errstr;
	char		*w;
	int		 n;

	if ((w = next_word(p)) == NULL) {
		bad("missing number");
		return;
	}
	n = strtonum(w, min, max, &errstr);
	if (errstr != NULL) {
		bad("number \"%s\" is %s (allowed: %d to %d)", w, errstr,
		    min, max);
		return;
	}
	if (no_more_words(p))
		*out = n;
}

/* Colors are checked when scrollman connects to X; see x_color(). */
static void
parse_color(char **p, const char **out)
{
	char	*w;

	if ((w = next_word(p)) == NULL) {
		bad("missing color");
		return;
	}
	if (!no_more_words(p))
		return;
	if ((*out = strdup(w)) == NULL)
		err(1, "strdup");
}

static void
parse_presets(char **p)
{
	int		 presets[MAX_WIDTH_PRESETS];
	const char	*errstr;
	char		*w;
	size_t		 n = 0;

	while ((w = next_word(p)) != NULL) {
		if (n == MAX_WIDTH_PRESETS) {
			bad("at most %d presets", MAX_WIDTH_PRESETS);
			return;
		}
		presets[n] = strtonum(w, 10, 100, &errstr);
		if (errstr != NULL) {
			bad("preset \"%s\" is %s (allowed: 10 to 100)", w,
			    errstr);
			return;
		}
		n++;
	}
	if (n == 0) {
		bad("missing presets");
		return;
	}
	memcpy(config.width_presets, presets, n * sizeof(presets[0]));
	config.nwidth_presets = n;
}

static void
parse_yes_no(char **p, int *out)
{
	char	*w;

	if ((w = next_word(p)) == NULL ||
	    (strcmp(w, "yes") != 0 && strcmp(w, "no") != 0)) {
		bad("expected yes or no");
		return;
	}
	if (no_more_words(p))
		*out = strcmp(w, "yes") == 0;
}

static void
parse_line(char *line)
{
	char	*p = line, *kw;

	line[strcspn(line, "\n")] = '\0';
	if ((kw = next_word(&p)) == NULL || kw[0] == '#')
		return;

	if (strcmp(kw, "bind") == 0)
		parse_bind(&p);
	else if (strcmp(kw, "unbind") == 0)
		parse_unbind(&p);
	else if (strcmp(kw, "border-width") == 0)
		parse_number(&p, 0, 100, &config.border_width);
	else if (strcmp(kw, "border-focus") == 0)
		parse_color(&p, &config.border_focus);
	else if (strcmp(kw, "border-normal") == 0)
		parse_color(&p, &config.border_normal);
	else if (strcmp(kw, "column-width") == 0)
		parse_number(&p, 10, 100, &config.column_percent);
	else if (strcmp(kw, "width-presets") == 0)
		parse_presets(&p);
	else if (strcmp(kw, "width-step") == 0)
		parse_number(&p, 1, 100, &config.width_step);
	else if (strcmp(kw, "focus-follows-mouse") == 0)
		parse_yes_no(&p, &config.focus_follows_mouse);
	else
		bad("unknown keyword \"%s\"", kw);
}

/*
 * Start from the built-in defaults, then apply the file at path if
 * there is one. A missing file is only reported if must_exist is set.
 * Bad lines are reported and skipped, so a typo never stops a login.
 */
void
config_load(const char *path, int must_exist)
{
	FILE	*f;
	char	*line = NULL;
	size_t	 size = 0;

	nbindings = sizeof(default_bindings) / sizeof(default_bindings[0]);
	if ((bindings = reallocarray(NULL, nbindings, sizeof(*bindings))) ==
	    NULL)
		err(1, "reallocarray");
	memcpy(bindings, default_bindings, sizeof(default_bindings));

	if (path == NULL)
		return;
	if ((f = fopen(path, "r")) == NULL) {
		if (must_exist || errno != ENOENT)
			warn("%s", path);
		return;
	}

	cur_path = path;
	cur_line = 0;
	while (getline(&line, &size, f) != -1) {
		cur_line++;
		parse_line(line);
	}
	free(line);
	fclose(f);
}
