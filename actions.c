#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "actions.h"
#include "config.h"
#include "layout.h"
#include "scrollman.h"
#include "x11.h"

void
act_spawn(const char *cmd)
{
	switch (fork()) {
	case -1:
		warn("fork");
		return;
	case 0:
		close(ConnectionNumber(wm.dpy));
		setsid();
		/* main() ignores SIGCHLD; the new program must not inherit that. */
		signal(SIGCHLD, SIG_DFL);
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		warn("execl %s", cmd);
		_exit(1);
	}
}

void
act_focus_left(const char *arg)
{
	struct column	*col;

	if (wm.ws->focus == NULL)
		return;
	if ((col = TAILQ_PREV(wm.ws->focus, column_q, entry)) != NULL)
		focus(col->focus);
}

void
act_focus_right(const char *arg)
{
	struct column	*col;

	if (wm.ws->focus == NULL)
		return;
	if ((col = TAILQ_NEXT(wm.ws->focus, entry)) != NULL)
		focus(col->focus);
}

void
act_focus_up(const char *arg)
{
	struct client	*c;

	if ((c = selected()) == NULL)
		return;
	if ((c = TAILQ_PREV(c, client_q, entry)) != NULL)
		focus(c);
}

void
act_focus_down(const char *arg)
{
	struct client	*c;

	if ((c = selected()) == NULL)
		return;
	if ((c = TAILQ_NEXT(c, entry)) != NULL)
		focus(c);
}

void
act_move_column_left(const char *arg)
{
	struct column	*col = wm.ws->focus;
	struct column	*prev;

	if (col == NULL || (prev = TAILQ_PREV(col, column_q, entry)) == NULL)
		return;
	TAILQ_REMOVE(&wm.ws->columns, col, entry);
	TAILQ_INSERT_BEFORE(prev, col, entry);
	focus(col->focus);
}

void
act_move_column_right(const char *arg)
{
	struct column	*col = wm.ws->focus;
	struct column	*next;

	if (col == NULL || (next = TAILQ_NEXT(col, entry)) == NULL)
		return;
	TAILQ_REMOVE(&wm.ws->columns, col, entry);
	TAILQ_INSERT_AFTER(&wm.ws->columns, next, col, entry);
	focus(col->focus);
}

void
act_move_window_up(const char *arg)
{
	struct client	*c, *prev;

	if ((c = selected()) == NULL || c->col == NULL ||
	    (prev = TAILQ_PREV(c, client_q, entry)) == NULL)
		return;
	TAILQ_REMOVE(&c->col->clients, c, entry);
	TAILQ_INSERT_BEFORE(prev, c, entry);
	focus(c);
}

void
act_move_window_down(const char *arg)
{
	struct client	*c, *next;

	if ((c = selected()) == NULL || c->col == NULL ||
	    (next = TAILQ_NEXT(c, entry)) == NULL)
		return;
	TAILQ_REMOVE(&c->col->clients, c, entry);
	TAILQ_INSERT_AFTER(&c->col->clients, next, c, entry);
	focus(c);
}

void
act_consume_or_expel_left(const char *arg)
{
	struct client	*c;

	if ((c = selected()) != NULL && c->col != NULL)
		consume_or_expel(c, 0);
}

void
act_consume_or_expel_right(const char *arg)
{
	struct client	*c;

	if ((c = selected()) != NULL && c->col != NULL)
		consume_or_expel(c, 1);
}

void
act_focus_workspace_up(const char *arg)
{
	struct workspace	*ws;

	if ((ws = TAILQ_PREV(wm.ws, workspace_q, entry)) != NULL)
		workspace_focus(ws);
}

void
act_focus_workspace_down(const char *arg)
{
	struct workspace	*ws;

	if ((ws = TAILQ_NEXT(wm.ws, entry)) != NULL)
		workspace_focus(ws);
}

void
act_move_column_to_workspace_up(const char *arg)
{
	struct workspace	*ws;

	if (wm.ws->focus == NULL ||
	    (ws = TAILQ_PREV(wm.ws, workspace_q, entry)) == NULL)
		return;
	column_to_workspace(wm.ws->focus, ws);
}

void
act_move_column_to_workspace_down(const char *arg)
{
	struct workspace	*ws;

	if (wm.ws->focus == NULL || (ws = TAILQ_NEXT(wm.ws, entry)) == NULL)
		return;
	column_to_workspace(wm.ws->focus, ws);
}

/* arg is the workspace number, from 1 to 10. */
void
act_focus_workspace(const char *arg)
{
	workspace_focus(workspace_nth(strtonum(arg, 1, 10, NULL)));
}

void
act_move_window_to_workspace(const char *arg)
{
	struct client	*c;

	if ((c = selected()) != NULL)
		client_to_workspace(c,
		    workspace_nth(strtonum(arg, 1, 10, NULL)));
}

/* Step to the next wider preset, or back to the narrowest. */
void
act_cycle_width(const char *arg)
{
	struct column	*col = wm.ws->focus;
	size_t		 i;
	int		 w;

	if (col == NULL)
		return;
	w = wm.sw * config.width_presets[0] / 100;
	for (i = 0; i < config.nwidth_presets; i++) {
		if (wm.sw * config.width_presets[i] / 100 > col->width) {
			w = wm.sw * config.width_presets[i] / 100;
			break;
		}
	}
	col->width = w;
	col->prev_width = 0;
	focus(col->focus);
}

void
act_full_width(const char *arg)
{
	struct column	*col = wm.ws->focus;

	if (col == NULL)
		return;
	if (col->prev_width != 0) {
		col->width = col->prev_width;
		col->prev_width = 0;
	} else {
		col->prev_width = col->width;
		col->width = wm.sw;
	}
	focus(col->focus);
}

/* Change the column's width by step % of the screen, within one step and full. */
static void
resize_column(int step)
{
	struct column	*col = wm.ws->focus;
	int		 w, min = wm.sw * config.width_step / 100;

	if (col == NULL)
		return;
	w = col->width + wm.sw * step / 100;
	if (w < min)
		w = min;
	if (w > wm.sw)
		w = wm.sw;
	col->width = w;
	col->prev_width = 0;
	focus(col->focus);
}

void
act_shrink_width(const char *arg)
{
	resize_column(-config.width_step);
}

void
act_grow_width(const char *arg)
{
	resize_column(config.width_step);
}

void
act_fullscreen(const char *arg)
{
	struct client	*c;

	if ((c = selected()) != NULL)
		set_fullscreen(c, !c->fullscreen);
}

void
act_close(const char *arg)
{
	struct client	*c;

	if ((c = selected()) != NULL)
		x_close(c->win);
}

void
act_quit(const char *arg)
{
	wm.running = 0;
}

void
act_restart(const char *arg)
{
	wm.restart = 1;
	wm.running = 0;
}
