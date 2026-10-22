#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>

void		 x_setup(void);
void		 x_scan(void);
void		 x_cleanup(void);
void		 x_grab_keys(void);
unsigned int	 x_clean_mask(unsigned int);
void		 x_close(Window);
int		 x_has_atom(Window, Atom, Atom);
void		 x_set_fullscreen(Window, int);
void		 x_set_active(Window);
void		 x_set_wm_state(Window, long);

#endif
