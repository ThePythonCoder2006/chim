#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "move_ptr.h"

#include "log.h"
#include "mol.h"

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
 * (*s) will point to the first char before the bond : right before '-' or '=' or '~' or '<' or '>'
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