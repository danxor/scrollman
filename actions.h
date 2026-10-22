#ifndef ACTIONS_H
#define ACTIONS_H

void	act_spawn(const char *);
void	act_focus_left(const char *);
void	act_focus_right(const char *);
void	act_focus_up(const char *);
void	act_focus_down(const char *);
void	act_move_column_left(const char *);
void	act_move_column_right(const char *);
void	act_move_window_up(const char *);
void	act_move_window_down(const char *);
void	act_consume_or_expel_left(const char *);
void	act_consume_or_expel_right(const char *);
void	act_focus_workspace_up(const char *);
void	act_focus_workspace_down(const char *);
void	act_move_column_to_workspace_up(const char *);
void	act_move_column_to_workspace_down(const char *);
void	act_focus_workspace(const char *);
void	act_move_window_to_workspace(const char *);
void	act_cycle_width(const char *);
void	act_full_width(const char *);
void	act_shrink_width(const char *);
void	act_grow_width(const char *);
void	act_fullscreen(const char *);
void	act_close(const char *);
void	act_quit(const char *);
void	act_restart(const char *);

#endif
