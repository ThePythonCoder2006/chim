#ifndef __CURSOR__
#define __CURSOR__

#include <stdint.h>

enum
{
  MODE_MOVE,
  MODE_BR_SELECT
};

#define CURSOR_MAX_CYCLE_DEPTH 32

typedef struct cursor_e
{
  char *pos;
  char *brptr;
  uint32_t brptr_cycle;
  uint8_t mode;
  uint32_t in_cycle;
  uint32_t curr_cycle_idx;
  uint32_t cycles_length[CURSOR_MAX_CYCLE_DEPTH];
} cursor;

#define DRAW_CURSOR_TEXT_LEFT 0
#define DRAW_CURSOR_TEXT_RIGHT 2

typedef struct draw_cursor_e
{
  double width[2];
  Vector2 pos;
  double atom_width;
  char text[4];
} draw_cursor;

#define BASE_DRAW_CURSOR "[\0]"

int handle_key_press(int keycode, cursor *cursor, char *mol);

int skip_to_closing_paren(char **s);

int skip_next_angle(char **s);
int skip_next_bond(char **s);
int skip_next_branch(char **s);
int skip_next_atom(char **s);

int skip_prev_angle(char **s);
int skip_prev_bond(char **s);
int skip_prev_branch(char **s);
int skip_prev_atom(char **s);

#endif // __CURSOR__