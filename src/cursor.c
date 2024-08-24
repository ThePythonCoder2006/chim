#include <stdio.h>
#include <ctype.h>

#include "raylib.h"

#include "cursor.h"
#include "mol.h"
#include "log.h"

static int cursor_skip_to_next_atom(cursor *cursor);
static int cursor_skip_to_first_branch(cursor *cursor);
static int cursor_skip_to_cycle_beginning(cursor *cursor);
static int cursor_skip_prev_to_cycle_beginning(cursor *cursor);

int skip_next_angle(char **s);
int skip_next_bond(char **s);
int skip_next_branch(char **s);
int skip_next_atom(char **s);
int skip_to_cycle_beginning(char **s, uint32_t *n);

int skip_prev_angle(char **s);
int skip_prev_bond(char **s);
int skip_prev_branch(char **s);
int skip_prev_atom(char **s);

/*
 * returns the keycode iff it wasn't interpreted, else returns 0
 */
int handle_key_press(int keycode, cursor *cursor, char *mol)
{
  if (cursor == NULL)
  {
    eprintf("Provided cursor pointer was NULL!\n");
    return -1;
  }

  if (mol == NULL)
  {
    eprintf("Provided molecule string was NULL!\n");
    return -1;
  }

  switch (keycode)
  {
  case KEY_J:
    if (cursor->mode == MODE_MOVE)
    {
      if (cursor_skip_to_next_atom(cursor))
        return -1;
    }
    else if (cursor->mode == MODE_BR_SELECT)
    {
      cursor->pos = cursor->brptr;
      cursor->cycles_length[0] = cursor->brptr_cycle;
      cursor->in_cycle += (cursor->brptr_cycle > 0);
      ++(cursor->pos); // skip the '('
      if (*cursor->pos == '[')
        if (skip_next_angle(&cursor->pos))
          return -1;
      if (skip_next_bond(&cursor->pos))
        return -1;
      cursor->mode = MODE_MOVE;
    }
    else
      __builtin_unreachable();
    break;

  case KEY_K:
    cursor->mode = MODE_MOVE;
    char *old = cursor->pos;
    --(cursor->pos);
    if (*cursor->pos == '\0')
    // first atom of the molecule
    {
      ++(cursor->pos);
      return 0;
    }
    if (skip_prev_bond(&cursor->pos))
    {
      cursor->pos = old;
      return 0;
    }

    // skip branch angle
    if (*cursor->pos == ']')
    {
      char *saved_pos = cursor->pos;
      skip_prev_angle(&cursor->pos);
      if (*cursor->pos != '(')
        cursor->pos = saved_pos;
    }

    if (*cursor->pos == '(' && !isdigit(cursor->pos[-1]))
      // exit branch if we are at the edge of one
      --(cursor->pos);

    while (*cursor->pos != '\0' && *cursor->pos == ')')
    {
      if (skip_prev_branch(&cursor->pos))
        return -1;
    }

    if (cursor->pos[1] == '(' && isdigit(cursor->pos[0]))
    {
      --(cursor->in_cycle);

      --(cursor->pos); // skip said '('
      // this is the first bond of a cycle
      // we need to leave the cycle to get to the attaching atom of the cycle
      while (*cursor->pos != '\0' && isdigit(*cursor->pos))
        --(cursor->pos);

      if (*cursor->pos != '*')
      {
        eprintf("Expected '*' as Cycle start symbol, but got %c\n", *cursor->pos);
        return -1;
      }
      --(cursor->pos); // skip said '*'

      if (*cursor->pos == '*')
        --(cursor->pos); // skip '*' for an arced cycle

      if (skip_prev_atom(&cursor->pos))
        return -1;

      break;
    }

    // if (IS_SPECIAL_CHAR(cursor->pos[-1]))
    //   ++(cursor->pos);

    if (skip_prev_atom(&cursor->pos))
      return -1;
    break;

  case KEY_L:
    if (cursor->mode != MODE_BR_SELECT)
    {
      if (cursor_skip_to_first_branch(cursor))
        return -1;
    }
    else
    {
      if (skip_next_branch(&cursor->brptr))
        return -1;
      if (*cursor->brptr != '(')
      {
        // fell of the last branch -> go back to the first branch
        cursor->brptr = cursor->pos;
        if (skip_next_atom(&cursor->brptr))
          return -1;
      }
    }
    break;

  case KEY_H:
    if (cursor->mode != MODE_BR_SELECT)
    {
      cursor->brptr = cursor->pos;
      if (skip_next_atom(&cursor->brptr))
        return -1;

      if (*cursor->brptr != '(')
      {
        cursor->brptr = cursor->pos;
        cursor->mode = MODE_MOVE;
      }
      else
      {
        cursor->mode = MODE_BR_SELECT;

        // skip to the last branch
        char *prevpos;
        do
        {
          prevpos = cursor->brptr;
          if (skip_next_branch(&cursor->brptr))
            return -1;
        } while (*cursor->brptr == '(');
        cursor->brptr = prevpos;
      }
    }
    else
    {
      --(cursor->brptr);
      if (*cursor->brptr != ')')
      {
        ++(cursor->brptr);
        // skip to the last branch
        char *prevpos;
        do
        {
          prevpos = cursor->brptr;
          if (skip_next_branch(&cursor->brptr))
            return -1;
        } while (*cursor->brptr == '(');
        cursor->brptr = prevpos;
      }
      else if (skip_prev_branch(&cursor->brptr))
        return -1;
    }
    break;

  default:
    return keycode;
  }

  printf("%i|%u/%u: %s\t|\t %i: %s\n", cursor->in_cycle, cursor->curr_cycle_idx, cursor->cycles_length[0], cursor->pos, cursor->brptr_cycle, cursor->brptr);

  return 0;
}

static int cursor_skip_to_next_atom(cursor *cursor)
{
  cursor->mode = MODE_MOVE;
  char *old = cursor->pos;
  if (skip_next_atom(&cursor->pos))
    return -1;

  if (*cursor->pos == '\0' || *cursor->pos == ')')
  {
    cursor->pos = old;
    return 0;
  }

  if (*cursor->pos == '*')
  {
    ++(cursor->pos);

    if (*cursor->pos == '*')
      ++(cursor->pos); // skip arced cycle's marker

    int chars_read;
    sscanf(cursor->pos, "%*i%n", &chars_read);
    cursor->pos += chars_read;
  }

  uint8_t skiped_branch = 0;
  while (*cursor->pos == '(')
  {
    skiped_branch = 1;
    if (skip_next_branch(&cursor->pos))
      return -1;
  }

  if (*cursor->pos == '\0')
  {
    cursor->pos = old;
    if (skiped_branch)
    {
      if (*cursor->pos == '*')
      {
        ++(cursor->pos);

        if (*cursor->pos == '*')
          ++(cursor->pos); // skip arced cycle's marker

        int chars_read;
        sscanf(cursor->pos, "%*i%n", &chars_read);
        cursor->pos += chars_read;

        ++cursor->in_cycle;
      }

      if (*cursor->pos != '(')
      {
        eprintf("Expected '(' as branch/cycle's start character, but got '%c'\n", *cursor->pos);
        return -1;
      }
      ++(cursor->pos);

      if (*cursor->pos == '[')
        if (skip_next_angle(&cursor->pos))
          return -1;

      if (skip_next_bond(&cursor->pos))
        return -1;
    }

    return 0;
  }

  if (skip_next_bond(&cursor->pos))
    return -1;

  if (cursor->in_cycle)
    ++(cursor->curr_cycle_idx);

  if (*cursor->pos == ')' && cursor->in_cycle)
    cursor_skip_prev_to_cycle_beginning(cursor);

  return 0;
}

static int cursor_skip_to_first_branch(cursor *cursor)
{
  cursor->brptr = cursor->pos;

  if (skip_next_atom(&cursor->brptr))
    return -1;

  if (*cursor->brptr == '*')
  {
    if (skip_to_cycle_beginning(&cursor->brptr, &cursor->brptr_cycle))
      return -1;
    --(cursor->brptr);
  }

  cursor->mode = MODE_MOVE;
  if (*cursor->brptr != '(')
    cursor->brptr = cursor->pos;
  else
    cursor->mode = MODE_BR_SELECT;

  return 0;
}
static int cursor_skip_to_cycle_beginning(cursor *cursor)
{
  if (skip_to_cycle_beginning(&cursor->pos, &cursor->cycles_length[0]))
    return -1;
  ++(cursor->in_cycle);

  return 0;
}

int skip_to_cycle_beginning(char **s, uint32_t *n)
{
  if (**s != '*')
  {
    eprintf("Expected '*' as start symbol for Cycle, but got '%c'\n", **s);
    return -1;
  }
  ++(*s); // skip said '*'

  if (**s == '*')
    ++(*s);

  if (**s == '[')
  {
    do
      ++(*s);
    while (**s != ']' && **s != '\0');
    ++(*s); // skip the final ']'
  }

  int char_count = 0;
  if (sscanf(*s, "%u%n", n, &char_count) < 1)
  {
    eprintf("Could not read the length of the cycle\n"
            "\t" COLORED_INFO " current ptr: \"%s\"\n",
            *s);
    return -1;
  }
  *s += char_count;

  if (**s != '(')
  {
    eprintf("Expected '(' to start Cycle's code, but got '%c'\n", **s);
    return -1;
  }
  ++(*s); // skip said '('

  return 0;
}

static int cursor_skip_prev_to_cycle_beginning(cursor *cursor)
{
  if (skip_to_closing_paren(&cursor->pos))
    return -1;

  if (skip_prev_branch(&cursor->pos))
    return -1;

  cursor->pos += 2;

  // // skip side number
  // while (isdigit(*cursor->pos))
  //   --(cursor->pos);

  // if (*cursor->pos == ']')
  // {
  //   // skip arc params
  //   while (*cursor->pos != '[')
  //     --(cursor->pos);
  //   --(cursor->pos);

  //   if (*cursor->pos != '*')
  //   {
  //     eprintf("Expected '*' as arced Cycle's maker, but got '%c'\n", *cursor->pos);
  //     return -1;
  //   }
  // }

  // if (*cursor->pos != '*')
  // {
  //   eprintf("Expected '*' as Cycle's start marker, but got '%c'\n", *cursor->pos);
  //   return -1;
  // }
  // --(cursor->pos);

  // if (*cursor->pos == '*')
  //   --(cursor->pos); // skip arced cycle's '*'

  // if (skip_prev_atom(&cursor->pos))
  //   return -1;

  return 0;
}

int skip_to_closing_paren(char **s)
{
  // paren starts at 1 because one was opened before
  for (uint32_t paren = 1; paren > 0; ++(*s))
  {
    if (**s == '\0')
    {
      eprintf("string ended before reaching closing parentesis !\n");
      return -1;
    }

    if (**s == '(')
      ++paren;
    else if (**s == ')')
      --paren;
  }
  --(*s);

  return 0;
}

// ----------- skip next ----------

int skip_next_angle(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer was NULL !\n");
    return -1;
  }

  if (**s != '[')
  {
    eprintf("Expected '[' as angle start symbol, but got %c\n", **s);
    return -1;
  }
  ++(*s); // skip '['

  while (**s != ']')
    ++(*s);

  ++(*s); // skip the final ']'

  return 0;
}

int skip_next_bond(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer was NULL !\n");
    return -1;
  }

  if (!IS_BOND_CHAR(**s))
  {
    eprintf("Unexpected bond character : %c\n", **s);
    return -1;
  }
  if (IS_CRAM_BOND_CHAR(**s) && (*s)[1] == ':')
    ++(*s); // skip ':'
  ++(*s);   // skip '-' or '=' or '~' or '<' or '>'

  if (**s != '[')
    return 0; // bond with no angle

  return skip_next_angle(s);
}

int skip_next_branch(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer was NULL !\n");
    return -1;
  }

  if (**s != '(')
  {
    eprintf("Expected '(' to start the branch but got %c\n", **s);
    return -1;
  }
  ++(*s);

  if (skip_to_closing_paren(s))
    return -1;
  ++(*s);
  return 0;
}

int skip_next_atom(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer was NULL !\n");
    return -1;
  }

  while (**s != 0 && !IS_SPECIAL_CHAR(**s))
    ++(*s);

  return 0;
}

// ------- skip prev --------------

/*
 * (*s) should point to the last char of the angle : ']'
 * (*s) will point to the first char before the angle : right before '['
 */
int skip_prev_angle(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer was NULL !\n");
    return -1;
  }

  if (**s == '\0')
  {
    eprintf("Provided string ended before angle started\n");
    return -1;
  }

  if (**s != ']')
  {
    eprintf("Expected ']' as closing symbol for angle, but got %c\n", **s);
    return -1;
  }
  --(*s); // skip said ']'

  while (**s != '\0' && **s != '[')
    --(*s);

  if (**s != '[')
  {
    eprintf("Expected '[' as openning symbol for angle, but got %c\n", **s);
    return -1;
  }
  --(*s); // skip said '['

  return 0;
}

/*
 * (*s) should point to the last char of the bond (usually ']')
 */
int skip_prev_bond(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer is NULL !\n");
    return -1;
  }

  if (**s == '\0')
  {
    eprintf("Provided string ended before any bond\n");
    return -1;
  }

  if (**s == ']')
  {
    if (skip_prev_angle(s))
      return -1;
  }

  if (**s == ':')
    --(*s); // skip the ':' from dashed cram bonds

  if (!IS_BOND_CHAR(**s))
  {
    eprintf("Unexpected bond char : %c\n", **s);
    return -1;
  }
  --(*s);

  return 0;
}

/*
 * (*s) should point to the closing ')' char of the branch
 * (*s) will point to the first char before the openning '(' char of the branch
 */
int skip_prev_branch(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer is NULL !\n");
    return -1;
  }

  if (**s == '\0')
  {
    eprintf("Provided string ended before any branch\n");
    return -1;
  }

  if (**s != ')')
  {
    eprintf("Expected ')' as an ending for a branch, but got %c\n", **s);
    return -1;
  }
  --(*s); // skip said '('

  uint32_t paren = 1; // 1 to account for this first paren

  while (paren > 0 && **s != '\0')
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
 * (*s) should point to the last char of the atom
 *    if atom is empty and is at the start of the molecule, (*s) is allowed to point to '\0'
 * (*s) will point to the first char of the atom
 */
int skip_prev_atom(char **s)
{
  if (s == NULL || *s == NULL)
  {
    eprintf("Provided string or string pointer is NULL !\n");
    return -1;
  }

  while (!IS_SPECIAL_CHAR(**s) && **s != '\0')
    --(*s);

  ++(*s); // to go back to the first char of the atom

  return 0;
}