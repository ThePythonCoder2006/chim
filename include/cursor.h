#ifndef __CURSOR__
#define __CURSOR__

#include <stdint.h>

enum
{
  MODE_MOVE,
  MODE_BR_SELECT
};

#define CURSOR_MAX_CYCLE_DEPTH 32

typedef struct cursor_cycle_info_s
{
  char *pos;
  uint32_t cycle_cnt;
  uint32_t brnch_cnt;
  uint32_t cycle_idx;
  uint32_t cycle_length;
} cursor_cycle_info;

typedef struct cursor_s
{
  cursor_cycle_info pos;
  cursor_cycle_info branch;
  uint8_t mode;
} cursor;

typedef enum CURSOR_MOVE_SELECT_e
{
  MOVE_POS,
  MOVE_BRPTR,
} CURSOR_MOVE_SELECT;

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

int handle_key_press(int keycode, cursor *cursor);

#endif // __CURSOR__