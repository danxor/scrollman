#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#include <X11/Xlib.h>

#define DEFAULT_BORDER_FOCUS	"#7fc8ff"
#define DEFAULT_BORDER_NORMAL	"#505050"
#define MAX_WIDTH_PRESETS	8

struct binding {
	unsigned int	 mod;
	KeySym		 keysym;
	void		(*fn)(const char *);
	const char	*arg;
};

struct config {
	int		 border_width;
	const char	*border_focus;
	const char	*border_normal;
	int		 column_percent;	/* new column width, % of screen */
	int		 width_step;		/* shrink/grow step, % of screen */
	int		 width_presets[MAX_WIDTH_PRESETS];	/* % of screen */
	size_t		 nwidth_presets;
	int		 focus_follows_mouse;
};

extern struct config	 config;
extern struct binding	*bindings;
extern size_t		 nbindings;

void	config_load(const char *, int);

#endif
