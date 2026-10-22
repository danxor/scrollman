#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "events.h"
#include "layout.h"
#include "scrollman.h"
#include "x11.h"

struct wm wm;

static void __dead
usage(void)
{
	fprintf(stderr, "usage: scrollman [-c file]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	char		 home_path[PATH_MAX];
	const char	*path = NULL, *home;
	int		 ch;

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			path = optarg;
			break;
		default:
			usage();
		}
	}
	if (optind != argc)
		usage();

	/* Let the kernel reap spawned programs. */
	signal(SIGCHLD, SIG_IGN);

	/* A file given with -c must exist; ~/.scrollmanrc may be missing. */
	if (path != NULL)
		config_load(path, 1);
	else if ((home = getenv("HOME")) != NULL) {
		snprintf(home_path, sizeof(home_path), "%s/.scrollmanrc", home);
		config_load(home_path, 0);
	} else
		config_load(NULL, 0);

	layout_init();
	x_setup();

	if (pledge("stdio proc exec", NULL) == -1)
		err(1, "pledge");

	x_scan();
	event_loop();
	x_cleanup();

	/* The windows stay open; the new scrollman picks them up in x_scan(). */
	if (wm.restart) {
		execvp(argv[0], argv);
		err(1, "execvp %s", argv[0]);
	}
	return (0);
}
