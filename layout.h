#ifndef LAYOUT_H
#define LAYOUT_H

#include <X11/Xlib.h>

struct client;
struct column;
struct workspace;

void		 layout_init(void);
struct workspace *workspace_nth(int);
void		 workspace_focus(struct workspace *);
void		 column_to_workspace(struct column *, struct workspace *);
void		 client_to_workspace(struct client *, struct workspace *);
struct client	*client_find(Window);
struct client	*selected(void);
void		 manage(Window);
void		 unmanage(struct client *);
void		 consume_or_expel(struct client *, int);
void		 set_fullscreen(struct client *, int);
void		 activate(struct client *);
void		 focus(struct client *);
void		 arrange(void);

#endif
