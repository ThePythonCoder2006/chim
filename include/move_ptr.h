#ifndef __MOVE_PTR__
#define __MOVE_PTR__

int skip_to_closing_paren(char **s);

int skip_next_angle(char **s);
int skip_next_bond(char **s);
int skip_next_branch(char **s);
int skip_next_atom(char **s);

int skip_prev_angle(char **s);
int skip_prev_bond(char **s);
int skip_prev_branch(char **s);
int skip_prev_atom(char **s);

#endif // __MOVE_PTR__