#ifndef __MOL__
#define __MOL__

#include "cursor.h"

enum angle_type_e
{
  ANGLE_RELATIVE,
  ANGLE_ABSOLUTE
};

typedef enum bond_type_e
{
  SINGLE_BOND,
  DOUBLE_BOND,
  TRIPLE_BOND,
  RIGHT_CRAM_FULL,
  LEFT_CRAM_FULL,
  RIGHT_CRAM_DASHED,
  LEFT_CRAM_DASHED
} bond_type;

typedef struct bond_e
{
  double angle;
  int length;
  bond_type type;
  double r_prev, r_next;
} bond;

#define BOND_LINE_THICKNESS 2
#define BOND_PADDING 0
#define BOND_LENGTH 50
#define DOUBLE_BOND_SPACING 4.0
#define CRAM_BASE_WIDTH 14.0
#define CRAM_DASH_BASE_SEP 4.0

#define MOL_BASE_FONT_SIZE 30

#define SUBSCRIPT_FONT_RATIO (2.0 / 3.0)

#define ATOM_MAX_LENGTH 256

typedef struct mol_info_s
{
  char *text;
  double base_angle;
  Vector2 start_pos;
  double font_size;
  Font font;
  Color color;
  cursor cursor;
  draw_cursor draw_cursor;
  uint8_t bond_drawn;
  uint8_t in_cycle;
  char atom[ATOM_MAX_LENGTH];
} mol_info;

typedef struct mol_work_data_s
{
  double atom_w;
  bond work_bond;
  Vector2 pos;
  char *work_ptr;
  uint8_t draw_cursor_should_be_updated;
} mol_work_data;

typedef struct mol_s
{
  mol_info info;
  mol_work_data work;
} mol;

#define IS_CRAM_BOND_CHAR(c) ((c) == '<' || (c) == '>')
#define IS_BOND_CHAR(c) ((c) == '-' || (c) == '=' || (c) == '~' || IS_CRAM_BOND_CHAR(c))
#define IS_SPECIAL_CHAR(c) (IS_BOND_CHAR(c) || (c) == '[' || (c) == ']' || (c) == '(' || (c) == ')' || (c) == '*' || (c) == ':')

#endif // __MOL