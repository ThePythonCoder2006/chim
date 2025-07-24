#include <ctype.h>

#include "raylib.h"

#include "move_ptr.h"
#include "cursor.h"
#include "log.h"
#include "mol.h"

int enter_cycle(char **s, uint32_t *length);
int cursor_enter_cycle(cursor *cursor, CURSOR_MOVE_SELECT mode);
int cursor_enter_branch(cursor *cursor, CURSOR_MOVE_SELECT mode);

int skip_prev_out_branch_start(char **s);
int cursor_skip_prev_out_branch_start(cursor *cursor);

int cursor_cycle_back_to_start_atom(cursor *cursor);
int cursor_skip_to_next_atom(cursor *cursor);
int cursor_skip_prev_out_cycle(cursor *cursor);
int cursor_skip_prev_to_last_atom(cursor *cursor);
int cursor_skip_to_first_branch(cursor *cursor, CURSOR_MOVE_SELECT mode);
int cursor_skip_to_last_branch(cursor *cursor, CURSOR_MOVE_SELECT mode);

int handle_key_press(int keycode, cursor *cursor)
{
  switch (keycode)
  {
  case KEY_J:
    if (cursor->mode == MODE_BR_SELECT)
    {
      cursor->pos = cursor->branch;
      if (cursor_enter_branch(cursor, MOVE_POS))
        return -1;

      cursor->mode = MODE_MOVE;
      break;
    }

    if (cursor->pos.cycle_cnt == cursor->pos.brnch_cnt && cursor->pos.cycle_idx == cursor->pos.cycle_length - 1)
    {
      if (cursor_cycle_back_to_start_atom(cursor))
        return -1;
      break;
    }

    if (cursor_skip_to_next_atom(cursor))
      return -1;

    break;

  case KEY_K:
    if (cursor_skip_prev_to_last_atom(cursor))
      return -1;
    break;

  case KEY_L:
    if (cursor->mode != MODE_BR_SELECT)
    {
      if (cursor_skip_to_first_branch(cursor, MOVE_BRPTR))
        return -1;
      break;
    }

    // mode MODE_BR_SELECT
    if (skip_next_branch(&cursor->branch.pos))
      return -1;

    if (*cursor->branch.pos != '(')
    {
      // fell of the last branch
      cursor->branch.pos = cursor->pos.pos;
      if (skip_next_atom(&cursor->branch.pos))
        return -1;
    }

    break;

  case KEY_H:
    if (cursor->mode != MODE_BR_SELECT)
    {
      cursor->branch = cursor->pos;
      if (skip_next_atom(&cursor->branch.pos))
        return -1;

      if (*cursor->pos.pos != '(' && *cursor->pos.pos != '*')
      {
        // there is no branch/cycle
        cursor->branch = cursor->pos;
        cursor->mode = MODE_MOVE;
        break;
      }

      if (cursor_skip_to_last_branch(cursor, MOVE_BRPTR))
        return -1;

      break;
    }

    /*
      (aaa...aaa)(bbb..bbb)
                 ^
                 |- cursor->branch.pos
    */
    --(cursor->branch.pos); // to check if there is a branch before or not
    if (*cursor->branch.pos != ')')
    {
      if (cursor_skip_to_last_branch(cursor, MOVE_BRPTR))
        return -1;
      break;
    }

    if (skip_prev_branch(&cursor->branch.pos))
      return -1;

    break;

  default:
    break;
  }

  printf("br%u:cy%u|%u/%u: %s\t|\t br%u:cy%u|%u/%u: %s\n",
         cursor->pos.brnch_cnt, cursor->pos.cycle_cnt, cursor->pos.cycle_idx, cursor->pos.cycle_length, cursor->pos.pos,
         cursor->branch.brnch_cnt, cursor->branch.cycle_cnt, cursor->branch.cycle_idx, cursor->branch.cycle_length, cursor->branch.pos);

  return 0;
}

int enter_cycle(char **s, uint32_t *length)
{
  if (**s != '*')
  {
    eprintf("Expected '*' as cycle start character, but got '%c'\n", **s);
    return -1;
  }

  ++(*s); // skip the '*'
  if (**s == '*')
  {
    ++(*s); // skip arced cycle's marker

    if (**s == '\0')
    {
      eprintf("Molecule ended before cycle's code !\n");
      return -1;
    }

    // skip the data of arced cycle
    if (**s == '[')
    {
      do
        ++(*s);
      while (**s != ']' && **s != '\0');
      ++(*s); // skip the final ']'
    }
  }

  int chars_read;
  if (sscanf(*s, "%u%n", length, &chars_read) < 1)
  {
    eprintf("Could not read the length of the cycle\n"
            "\t" COLORED_INFO "on ptr: \"%s\"\n",
            *s);
    return -1;
  }
  *s += chars_read;

  return 0;
}
int cursor_enter_cycle(cursor *cursor, CURSOR_MOVE_SELECT mode)
{
  cursor_cycle_info *curr;
  switch (mode)
  {
  case MOVE_POS:
    curr = &cursor->pos;
    break;
  case MOVE_BRPTR:
    curr = &cursor->branch;
    break;
  }

  if (enter_cycle(&curr->pos, &curr->cycle_length))
    return -1;

  ++(curr->cycle_cnt);
  curr->cycle_idx = 1;
  return 0;
}

int cursor_enter_branch(cursor *cursor, CURSOR_MOVE_SELECT mode)
{
  cursor_cycle_info *curr;
  switch (mode)
  {
  case MOVE_POS:
    curr = &cursor->pos;
    break;
  case MOVE_BRPTR:
    curr = &cursor->branch;
    break;
  }

  if (*curr->pos != '(')
  {
    eprintf("Expected '(' as branch start character, but got '%c'\n", *curr->pos);
    return -1;
  }
  ++(curr->pos);

  if (*curr->pos == '[')
    skip_next_angle(&curr->pos);

  if (skip_next_bond(&curr->pos))
    return -1;

  ++(curr->brnch_cnt);

  return 0;
}

int skip_prev_out_branch_start(char **s)
{
  int paren = 1; // for the closing paren already openned

  while (**s != '\0' && paren > 0)
  {
    if (**s == ')')
      ++paren;
    else if (**s == '(')
      --paren;
    --(*s);
  }

  return 0;
}

/*
 * *s should point to the end of an atom or branch
 * sets count to the number of atom + bond groups encountered before reaching the start of the branch
 */
int skip_prev_out_branch_start_count(char **s, uint32_t *count)
{
  *count = 1;
  while (**s != '\0')
  {
    while (**s == ')' && **s != '\0')
      if (skip_prev_branch(s))
        return -1;

    if (!IS_SPECIAL_CHAR(**s))
      if (skip_prev_atom(s))
        return -1;

    // skip bond, but we cant use `skip_prev_bond` for error handling

    if (**s == '[')
      if (skip_prev_angle(s))
        return -1;

    if (IS_BOND_CHAR(**s) && **s != '\0')
      --(*s);
    else if (**s != '(')
    {
      eprintf("Expected '(' as branch/cycle's start char, but got '%c'\n", **s);
      return -1;
    }
    ++(*count);

    if (**s == '(')
      break;
  }
  --(*s);

  return 0;
}

int cursor_skip_prev_out_branch_start(cursor *cursor)
{
  --(cursor->pos.brnch_cnt);
  return skip_prev_out_branch_start(&cursor->pos.pos);
}

int cursor_cycle_back_to_start_atom(cursor *cursor)
{
  // we are inside a cycle and at the end of one
  cursor->pos.cycle_idx = 0;

  if (*cursor->pos.pos == ')')
    --(cursor->pos.pos);
  if (cursor_skip_prev_out_branch_start(cursor))
    return -1;

  // skip the cycle start
  while (isdigit(*cursor->pos.pos))
    --(cursor->pos.pos);

  if (*cursor->pos.pos != '*')
  {
    eprintf("Expected '*' as cycle start character, but got '%c'\n", *cursor->pos.pos);
    return -1;
  }
  --(cursor->pos.pos); // skip '*' as cycle's start character

  if (*cursor->pos.pos == '*')
    --(cursor->pos.pos); // skip '*' as arced cycle's start character

  if (skip_prev_atom(&cursor->pos.pos))
    return -1;

  return 0;
}

int cursor_skip_to_next_atom(cursor *cursor)
{
  // store the pos in case we end up at the end of string/branch
  char *old_pos = cursor->pos.pos;

  if (skip_next_atom(&cursor->pos.pos))
    return -1;

  if (*cursor->pos.pos == '\0')
  {
    cursor->pos.pos = old_pos;
    return 0;
  }

  if (*cursor->pos.pos == ')')
  {
    if (cursor->pos.brnch_cnt <= 0)
    {
      eprintf("Expected cursor->pos.brnch_cnt to be positive: encountered ')'\n");
      return -1;
    }

    cursor->pos.pos = old_pos;
    return 0;
  }

  if (*cursor->pos.pos == '*')
  {
    if (cursor->pos.cycle_cnt > cursor->pos.brnch_cnt)
    {
      if (enter_cycle(&cursor->pos.pos, &cursor->pos.cycle_length))
        return -1;
      cursor->pos.cycle_idx = 1;
      if (cursor_enter_branch(cursor, MOVE_POS))
        return -1;
      return 0;
    }

    uint32_t temp;
    if (enter_cycle(&cursor->pos.pos, &temp))
      return -1;
  }

  while (*cursor->pos.pos == '(')
    if (skip_next_branch(&cursor->pos.pos))
      return -1;

  if (*cursor->pos.pos == '\0')
  {
    cursor->pos.pos = old_pos;
    TODO("Add support for backtracking into branches/cycles if there is nothing afterwards\n");
    return 0;
  }

  if (skip_next_bond(&cursor->pos.pos))
    return -1;

  if (cursor->pos.cycle_cnt >= cursor->pos.brnch_cnt && cursor->pos.cycle_cnt > 0)
    ++(cursor->pos.cycle_idx);

  return 0;
}

int cursor_skip_prev_out_cycle(cursor *cursor)
{
  --(cursor->pos.brnch_cnt);
  --(cursor->pos.cycle_cnt);

  // skip number
  while (*cursor->pos.pos != '\0' && isdigit(*cursor->pos.pos))
    --(cursor->pos.pos);

  if (*cursor->pos.pos != '*')
  {
    eprintf("Expected '*' as Cycle start symbol, but got %c\n", *cursor->pos.pos);
    return -1;
  }
  --(cursor->pos.pos); // skip said '*'

  if (*cursor->pos.pos == '*')
    --(cursor->pos.pos); // skip '*' for an arced cycle

  if (cursor->pos.cycle_cnt > 0)
  {
    char *new_cycle_length_getter = cursor->pos.pos;

    if (skip_prev_out_branch_start_count(&new_cycle_length_getter, &cursor->pos.cycle_idx))
      return -1;

    while (isdigit(*new_cycle_length_getter) && *new_cycle_length_getter != '\0')
      --new_cycle_length_getter;
    ++new_cycle_length_getter;

    if (sscanf(new_cycle_length_getter, "%u", &cursor->pos.cycle_length) < 1)
    {
      eprintf("Could not read the length of the cycle\n"
              "\t" COLORED_INFO "on ptr: \"%s\"\n",
              new_cycle_length_getter);
      return -1;
    }
  }

  return 0;
}

int cursor_skip_prev_to_last_atom(cursor *cursor)
{
  cursor->mode = MODE_MOVE;
  char *old_pos = cursor->pos.pos;
  --(cursor->pos.pos); // to check what comes before
  if (*cursor->pos.pos == '\0')
  {
    // first atom of the molecule
    ++(cursor->pos.pos);
    return 0;
  }

  // try catch philosophy
  if (skip_prev_bond(&cursor->pos.pos))
  {
    fprintf(stderr, COLORED_INFO PRINT_POS "This branch is actually being taken\n", __FILE__, __LINE__, __func__);
    cursor->pos.pos = old_pos;
    return 0;
  }

  if (*cursor->pos.pos == ']')
  {
    old_pos = cursor->pos.pos;
    skip_prev_angle(&cursor->pos.pos);
    if (*cursor->pos.pos != '(')
      cursor->pos.pos = old_pos;
  }

  if (*cursor->pos.pos == '(')
  {
    --(cursor->pos.pos);
    if (cursor->pos.brnch_cnt > cursor->pos.cycle_cnt)
      --(cursor->pos.brnch_cnt); // exit the branch
  }

  while (*cursor->pos.pos != '\0' && *cursor->pos.pos == ')')
    if (skip_prev_branch(&cursor->pos.pos))
      return -1;

  // some check are redundant
  if (cursor->pos.pos[1] == '(' &&
      isdigit(*cursor->pos.pos) &&
      cursor->pos.cycle_cnt >= cursor->pos.brnch_cnt &&
      cursor->pos.cycle_idx == 1)
    if (cursor_skip_prev_out_cycle(cursor))
      return -1;

  if (skip_prev_atom(&cursor->pos.pos))
    return -1;

  if (cursor->pos.cycle_cnt > cursor->pos.brnch_cnt)
    --(cursor->pos.cycle_cnt);
  if (cursor->pos.cycle_cnt == cursor->pos.brnch_cnt && cursor->pos.cycle_cnt > 0)
    --(cursor->pos.cycle_idx);

  return 0;
}

int cursor_skip_to_first_branch(cursor *cursor, CURSOR_MOVE_SELECT mode)
{
  cursor_cycle_info *curr = NULL;
  switch (mode)
  {
  case MOVE_POS:
    curr = &cursor->pos;
    break;
  case MOVE_BRPTR:
    curr = &cursor->branch;
    break;
  }

  char *old_pos = cursor->pos.pos;
  *curr = cursor->pos;

  if (skip_next_atom(&curr->pos))
    return -1;

  if (*curr->pos == '*')
  {
    printf("entered cycle!\n");
    if (cursor_enter_cycle(cursor, mode))
      return -1;
  }

  cursor->mode = MODE_MOVE;
  if (*curr->pos != '(')
    curr->pos = old_pos;
  else
    cursor->mode = MODE_BR_SELECT;

  return 0;
}

int cursor_skip_to_last_branch(cursor *cursor, CURSOR_MOVE_SELECT mode)
{
  cursor_cycle_info *curr = NULL;
  switch (mode)
  {
  case MOVE_POS:
    curr = &cursor->pos;
    break;

  case MOVE_BRPTR:
    curr = &cursor->branch;
    break;
  }

  cursor->mode = MODE_BR_SELECT;

  if (*curr->pos == '*')
    if (cursor_enter_cycle(cursor, mode))
      return -1;

  char *prev_pos;
  do
  {
    prev_pos = curr->pos;
    if (skip_next_branch(&curr->pos))
      return -1;

    if (*curr->pos == '*')
      if (cursor_enter_cycle(cursor, mode))
        return -1;

  } while (*curr->pos == '(');
  curr->pos = prev_pos;
  return 0;
}