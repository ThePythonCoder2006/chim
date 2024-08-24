#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define NaN NAN

#include "raylib.h"
#include "raymath.h"

#include "mol.h"
#include "cursor.h"
#include "log.h"

#define SWIDTH 1400
#define SHEIGHT 800

#define FIRA_CODE_FONT_PATH "fonts/Fira_Code/static/FiraCode-Regular.ttf"

void skip_white_space(char **s);

int get_next_atom(mol *mol);
int DrawMol(mol *mol, Vector2 start_pos, double angle, Font font);
int DrawMol_inside(mol *mol);
int DrawBranches(mol *mol);

int create_mol(mol *empty_mol, char *text);

char g_atom_buffer[ATOM_MAX_LENGTH] = {0};

int main(int argc, char **argv)
{
  (void)argc, (void)argv;

  mol mol = {.info.text = NULL};
  int r = -1; // if I forgot to uncomment a mol, this will error out by default

  // r = create_mol(&mol, "\0HO-[:90](=[:150]O)-[:30]N(-[:90]Ph)-[::-60](-[:-90]NO2)-[::60](-[:90]C6H5)-[::-60](=[:30]O)-[:-90]OH");
  // r = create_mol(&mol, "\0[:20]NO2-[:60](-[:120]Cl)=[:0](-[:60]Cl)-[:-60]");
  // r = create_mol(&mol, "\0(-[:90]Cl)(-[:-120]Cl)(<[:-60]Cl)(<:[:-30]Cl)-");
  // r = create_mol(&mol, "\0A-B(-[:45]W-X)-C");
  // r = create_mol(&mol, "\0[:75]R-C(=[::+60]O)-[::-60]O-[::-60]C(=[::+60]O)-[::-60]R'");
  // r = create_mol(&mol, "\0-[:0]{C6H10O6}10000-[:0]");
  // r = create_mol(&mol, "\0(-[:-150]X*6(-=-=-=))-[:-30](<[:-90]CH3)-[:30]N-[:-30]CH3");
  // r = create_mol(&mol, "\0H-C~N");
  // r = create_mol(&mol, "\0HC~CH");
  r = create_mol(&mol, "\0-**5(---)-[:90]");
  // r = create_mol(&mol, "\0X*5((-A=B-C)-C(-D-E)-C(=)-C(-F)-C(-G=)-)");
  // r = create_mol(&mol, "\0*6(-*5(-O--O-)=-=-=)");
  // r = create_mol(&mol, "\0A-B([:60]-D-E)([::-30]-X-Y)-C");
  // r = create_mol(&mol, "\0*5(--*6(-*4(-*5(----)--)----)---)");
  // r = create_mol(&mol, "\0A-B*5(-C-D*5(-X-Y-Z-)-E-F-)");
  // r = create_mol(&mol, "\0**6(-=-=-=)");
  // r = create_mol(&mol, "\0**[30,330]7(-------)");
  // r = create_mol(&mol, "\0HO-[:-30](=[:30]O)-[:-90]-[:-30]NH-[:-90](-[:-150]NH-[:150]*6(=-=(-C~N)-=-))=[:-30]N-[:30]-[:-30]*6(=-=-*5(-O--O-)=-)");
  // r = create_mol(&mol, "\0*5(---([::0]-A-B)--)");
  // r = create_mol(&mol, "\0(-[:90])-[:30]=[:-30]-[:30]([:-30]-CH3)");

  if (r)
  {
    eprintf("Creating molecule object failed or you forgot to uncomment a line\n\t" COLORED_INFO " mol.info.text = %s\n", mol.info.text);
    return -1;
  }

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SWIDTH, SHEIGHT, "Chim");
  SetTargetFPS(60);

  Font Fira = LoadFont(FIRA_CODE_FONT_PATH);

  BeginDrawing();
  ClearBackground(RAYWHITE);

  if (DrawMol(&mol, (Vector2){300, 400}, 0, Fira))
  {
    eprintf("Drawing failed !!!\n");
    return 1;
  }

  EndDrawing();

  while (!WindowShouldClose())
  {
    int keycode;
    if ((keycode = GetKeyPressed()) == 0)
    {
      BeginDrawing();
      EndDrawing();
      continue;
    }

    do
      if (handle_key_press(keycode, &mol.info.cursor, mol.info.text))
      {
        eprintf("Handling pressed keys has failed !!\n");
        return 1;
      }
    while ((keycode = GetKeyPressed()) != 0);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (DrawMol(&mol, (Vector2){300, 400}, 0, Fira))
    {
      eprintf("Drawing failed !!!\n");
      return 1;
    }

    EndDrawing();
  }
  CloseWindow();

  return 0;
}

double get_next_angleEx(char **s, enum angle_type_e *mode)
{
  // a valid angle info starts with a '['
  if (**s != '[')
  {
    eprintf("Expected '[' to start the angle but got %c\n", **s);
    return NaN;
  }
  ++(*s); // skip '['

  if (**s != ':')
  {
    eprintf("Expected ':' to start the angle but got %c\n[TODO] add support for predefined angle positions, that do not use ':'\n", **s);
    return NaN;
  }
  ++(*s); // skip ':'

  *mode = ANGLE_ABSOLUTE;
  if (**s == ':')
  {
    *mode = ANGLE_RELATIVE;
    ++(*s); // skip ':'
  }

  // read the angle in degrees
  double bond_angle = 0;
  int chars_read = 0;
  sscanf(*s, "%lf%n", &bond_angle, &chars_read);
  *s += chars_read;

  if (**s != ']')
  {
    eprintf("Expected ']' to end the angle but got %c\n", **s);
    return NaN;
  }
  ++(*s); // skip said ']'

  return bond_angle;
}

int get_next_angleMol(mol_work_data *mol_work)
{
  enum angle_type_e mode;
  double bond_angle = get_next_angleEx(&mol_work->work_ptr, &mode);
  if (__isnan(bond_angle))
    return -1;

  if (mode == ANGLE_ABSOLUTE)
    mol_work->work_bond.angle = bond_angle * PI / 180;
  else if (mode == ANGLE_RELATIVE)
    mol_work->work_bond.angle += bond_angle * PI / 180;
  else
    __builtin_unreachable();

  return 0;
}

int get_next_bondEx(mol *mol, double default_angle)
{
  // a valid bond must start with '-' or '='
  switch (*mol->work.work_ptr)
  {
  case '-':
    mol->work.work_bond.type = SINGLE_BOND;
    break;

  case '=':
    mol->work.work_bond.type = DOUBLE_BOND;
    break;

  case '~':
    mol->work.work_bond.type = TRIPLE_BOND;
    break;

  case '>':
    mol->work.work_bond.type = RIGHT_CRAM_FULL;
    if (mol->work.work_ptr[1] == ':')
    {
      mol->work.work_bond.type = RIGHT_CRAM_DASHED;
      ++(mol->work.work_ptr);
    }
    break;

  case '<':
    mol->work.work_bond.type = LEFT_CRAM_FULL;
    if (mol->work.work_ptr[1] == ':')
    {
      mol->work.work_bond.type = LEFT_CRAM_DASHED;
      ++(mol->work.work_ptr);
    }
    break;

  default:
    eprintf("Unexpected bond character : %c\n", *mol->work.work_ptr);
    return -1;
  }
  ++(mol->work.work_ptr); // skip '-' or '=' or '~' or '<' or '>'

  if (*mol->work.work_ptr != '[')
  {
    mol->work.work_bond.angle = default_angle;
    return 0; // bond with no info
  }

  // bond with info
  return get_next_angleMol(&mol->work);
}

int get_next_bond(mol *mol)
{
  return get_next_bondEx(mol, mol->info.base_angle);
}

/*
 * atom MUST be at least ATOM_MAX_LENGTH chars long, any further space will not be touched
 */
int get_next_atom(mol *mol)
{
  if (mol == NULL)
  {
    eprintf("provided mol pointer was NULL !\n");
    return -1;
  }

  mol->work.draw_cursor_should_be_updated = 0;
  if (mol->work.work_ptr == mol->info.cursor.pos)
    mol->work.draw_cursor_should_be_updated = 1;

  char *atom = mol->info.atom;

  // ensure that atom is zero terminated
  memset(atom, 0, ATOM_MAX_LENGTH * sizeof(atom[0]));

  while (*mol->work.work_ptr != '\0' && !IS_SPECIAL_CHAR(*mol->work.work_ptr))
  {
    *atom = *mol->work.work_ptr;
    ++(mol->work.work_ptr), ++(atom);
  }

  return 0;
}

#define POS_PLUS_POLAR(posx, posy, r, theta) posx + (r) * cos((theta)), posy - (r) * sin((theta))
#define POS_PLUS_POLAR_X(pos, r, theta) POS_PLUS_POLAR(pos, r, theta)
#define POS_PLUS_EGAL_POLAR(posx, posy, r, theta) POS_PLUS_POLAR(posx = posx, posy = posy, r, theta)
#define VEC2_POS_PLUS_POLAR(pos, r, theta) \
  (Vector2) { POS_PLUS_POLAR(((pos).x), ((pos).y), r, theta) }
#define VEC2_POS_PLUS_POLAR_X(pos, r, theta) VEC2_POS_PLUS_POLAR((pos), r, theta)

void DrawBond(mol_info *mol, bond b, Vector2 pos)
{
  switch (b.type)
  {
  case TRIPLE_BOND:
    // draw a third line
    DrawLineEx(VEC2_POS_PLUS_POLAR_X(
                   VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle),
                   DOUBLE_BOND_SPACING,
                   b.angle - PI / 2),
               VEC2_POS_PLUS_POLAR_X(
                   VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle),
                   DOUBLE_BOND_SPACING,
                   b.angle - PI / 2),
               BOND_LINE_THICKNESS, mol->color);
  // fallthrough
  case DOUBLE_BOND:
    // draw a second line
    DrawLineEx(VEC2_POS_PLUS_POLAR_X(
                   VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle),
                   DOUBLE_BOND_SPACING,
                   b.angle + PI / 2),
               VEC2_POS_PLUS_POLAR_X(
                   VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle),
                   DOUBLE_BOND_SPACING,
                   b.angle + PI / 2),
               BOND_LINE_THICKNESS, mol->color);
  // fallthrough
  case SINGLE_BOND:
    DrawLineEx(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle),
               VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle),
               BOND_LINE_THICKNESS, mol->color);
    break;

  case RIGHT_CRAM_FULL:
    DrawTriangle(
        VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle), CRAM_BASE_WIDTH / 2, b.angle + PI / 2),
        VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle), CRAM_BASE_WIDTH / 2, b.angle - PI / 2),
        VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle),
        mol->color);
    break;

  case LEFT_CRAM_FULL:
    DrawTriangle(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle),
                 VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle), CRAM_BASE_WIDTH / 2, b.angle - PI / 2),
                 VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.length - b.r_next, b.angle), CRAM_BASE_WIDTH / 2, b.angle + PI / 2),
                 mol->color);
    break;

    Vector2 top_pos;
    Vector2 bot_pos;
    double Delta_r_scalar, Delta_h_scalar;
    Vector2 Delta_r, Delta_h;
  case RIGHT_CRAM_DASHED:
    top_pos = VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle), CRAM_BASE_WIDTH / 2, b.angle + PI / 2);
    bot_pos = VEC2_POS_PLUS_POLAR_X(VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle), CRAM_BASE_WIDTH / 2, b.angle - PI / 2);
    Delta_r_scalar = CRAM_DASH_BASE_SEP;
    Delta_h_scalar = -(CRAM_BASE_WIDTH * CRAM_DASH_BASE_SEP) / (2.0 * b.length);
    Delta_r = (Vector2){POS_PLUS_POLAR(0, 0, Delta_r_scalar, b.angle)};
    Delta_h = (Vector2){POS_PLUS_POLAR(0, 0, Delta_h_scalar, b.angle)};

    for (uint32_t i = 0; CRAM_DASH_BASE_SEP * i <= b.length - b.r_next; ++i)
    {
      DrawLineEx(top_pos, bot_pos, BOND_LINE_THICKNESS, mol->color);
      top_pos = Vector2Add(top_pos, Vector2Add(Delta_r, Delta_h));
      bot_pos = Vector2Add(bot_pos, Vector2Subtract(Delta_r, Delta_h));
    }
    break;

  case LEFT_CRAM_DASHED:
    top_pos = VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle);
    bot_pos = VEC2_POS_PLUS_POLAR(pos, b.r_prev, b.angle);
    Delta_r_scalar = CRAM_DASH_BASE_SEP;
    Delta_h_scalar = (CRAM_BASE_WIDTH * CRAM_DASH_BASE_SEP) / (2.0 * b.length);
    Delta_r = (Vector2){POS_PLUS_POLAR(0, 0, Delta_r_scalar, b.angle)};
    Delta_h = (Vector2){POS_PLUS_POLAR(0, 0, Delta_h_scalar, b.angle + PI / 2)};

    for (uint32_t i = 0; CRAM_DASH_BASE_SEP * i <= b.length - b.r_next; ++i)
    {
      DrawLineEx(top_pos, bot_pos, BOND_LINE_THICKNESS, mol->color);
      top_pos = Vector2Add(top_pos, Vector2Subtract(Delta_r, Delta_h));
      bot_pos = Vector2Add(bot_pos, Vector2Add(Delta_r, Delta_h));
    }
    break;
  }

  mol->bond_drawn = 1;

  return;
}

void DrawBond_UpdatePos(mol *mol)
{
  DrawBond(&mol->info, mol->work.work_bond, mol->work.pos);

  mol->work.pos = VEC2_POS_PLUS_POLAR(mol->work.pos, mol->work.work_bond.length, mol->work.work_bond.angle);

  if (mol->work.draw_cursor_should_be_updated)
    mol->info.draw_cursor.pos = mol->work.pos;
}

double action_on_next_chunk(char **s, Vector2 center, mol_info *mol, double font_ratio,
                            int (*cond)(int),
                            void (*action)(Font, const char *, Vector2, float, float, Color))
{
  if (cond == NULL)
  {
    eprintf("provided condition function pointer was NULL\n");
    return NaN;
  }

  uint8_t i = 0;
  while ((*cond)(**s) && **s != '\0')
  {
    g_atom_buffer[i] = **s;
    ++(*s), ++i;
  }
  g_atom_buffer[i] = '\0';
  if (action != NULL)
    (*action)(mol->font, g_atom_buffer, center, mol->font_size *font_ratio, 0, mol->color);
  return MeasureTextEx(mol->font, g_atom_buffer, mol->font_size * font_ratio, 0).x;
}

double DrawNextChunk(char **s, Vector2 center, mol_info *mol, double font_ratio, int (*cond)(int))
{
  return action_on_next_chunk(s, center, mol, font_ratio, cond, &DrawTextEx);
}
double MeasureNextChunk(char **s, mol_info *mol, double font_ratio, int (*cond)(int))
{
  return action_on_next_chunk(s, (Vector2){0, 0}, mol, font_ratio, cond, NULL);
}

int not_isdigit(int c) { return !isdigit(c); }

double MeasureAtom(mol_info *mol)
{
  if (isdigit(mol->atom[0]))
  {
    eprintf("Expected a non-digit start character for atom, but got %c\n", mol->atom[0]);
    return NaN; // digit is invalid start char for atom
  }

  double acc = 0;
  char *atom = mol->atom; // needed for pointer aritmethic

  while (*atom != '\0')
  {
    acc += MeasureNextChunk(&atom, mol, 1, &not_isdigit);
    acc += MeasureNextChunk(&atom, mol, SUBSCRIPT_FONT_RATIO, &isdigit);
  }

  return acc;
}

int MeasureAtom_InPlace(mol *mol)
{
  mol->work.work_bond.r_prev = mol->work.work_bond.r_next;
  mol->work.work_bond.r_next = mol->work.atom_w = 0;
  if (mol->info.atom[0] != '\0')
  {
    if (__isnan(mol->work.atom_w = MeasureAtom(&mol->info)))
      return -1;
    mol->work.work_bond.r_next = __max(mol->work.atom_w / 2, mol->info.font_size / 2) + BOND_PADDING;
  }
  if (mol->work.draw_cursor_should_be_updated)
    mol->info.draw_cursor.atom_width = mol->work.atom_w;

  return 0;
}

int DrawAtom(mol *mol)
{
  if (isdigit(mol->info.atom[0]))
  {
    eprintf("Expected a non-digit starting char for atom, but got %c\n", mol->info.atom[0]);
    return -1; // digit is invalid start char for atom
  }

  char *atom = mol->info.atom; // needed for pointer aritmethic

  double w = MeasureAtom(&mol->info);

  Vector2 top_left = Vector2Subtract(mol->work.pos, (Vector2){w / 2, mol->info.font_size / 2});

  while (*atom != '\0')
  {
    top_left.x += DrawNextChunk(&atom, top_left, &mol->info, 1, &not_isdigit); // draws non digit characters : atom names
    top_left.x += DrawNextChunk(&atom,
                                Vector2Add(top_left, (Vector2){0, (2.0 / 5.0) * mol->info.font_size}),
                                &mol->info, SUBSCRIPT_FONT_RATIO, &isdigit); // draws digits : atom counts
  }

  return 0;
}

int DrawMolStep(mol *mol, double branch_angle_offset, double bond_default_angle, double prev_theta);
int DrawMolSpecialFeatures(mol *mol, double branch_angle_offset, double prev_theta);

/*
 * prev_theta MUST be non NaN iff this is not a nested cycle
 */
int DrawCycle(mol *mol, double prev_theta)
{
  if (mol == NULL)
  {
    eprintf("Provided mol pointer was NULL !\n");
    return -1;
  }

  if (mol->work.work_ptr == NULL)
  {
    eprintf("Provided string pointer was NULL !\n");
    return -1;
  }

  if (*mol->work.work_ptr != '*')
  {
    eprintf("Expected '*' as a Cycle start character, but got '%c'\n", *mol->work.work_ptr);
    return -1;
  }
  ++mol->work.work_ptr; // skip said '*'

  bond saved_bond = mol->work.work_bond;
  Vector2 saved_pos = mol->work.pos;

  uint8_t arced_cycle = 0;
  double arc_start_angle = 0, arc_end_angle = 360;
  if (*mol->work.work_ptr == '*')
  {
    arced_cycle = 1;
    ++(mol->work.work_ptr); // skip said '*'
    if (*mol->work.work_ptr == '[')
    // angle info
    {
      ++(mol->work.work_ptr); // skip said '['
      int char_count = -1;
      if (sscanf(mol->work.work_ptr, "%lf,%lf%n", &arc_start_angle, &arc_end_angle, &char_count) < 2)
      {
        eprintf("reading the cycle's arc start and/or end angle failed, current char is probably not a valid start for a number.\n"
                "\t" COLORED_INFO " Current working string is \"%s\"\n",
                mol->work.work_ptr);
        return -1;
      }
      mol->work.work_ptr += char_count;

      if (*mol->work.work_ptr != ']')
      {
        eprintf("Expected ']' as arc angle end character, but got '%c'\n", *mol->work.work_ptr);
        return -1;
      }
      ++(mol->work.work_ptr); // skip said ']'
    }
  }

  uint32_t n = 0;
  int char_count = -1;
  if (sscanf(mol->work.work_ptr, "%u%n", &n, &char_count) < 1)
  {
    eprintf("reading the cycle length failed, current char is probably not a valid start for a number.\n\t" COLORED_INFO " Current working string is \"%s\"\n", mol->work.work_ptr);
    return -1;
  }
  mol->work.work_ptr += char_count;
  const double theta = 2 * PI / n;

  // if we are already in a different cycle we need to do the angle a little different
  if (mol->info.in_cycle)
  {
    if (__isnan(prev_theta))
    {
      eprintf("prev_theta was NaN while this is a nested cycle !!\n");
      return -1;
    }
    mol->work.work_bond.angle -= PI - theta - prev_theta;
  }
  else if (mol->info.bond_drawn)
    mol->work.work_bond.angle -= (PI - theta) / 2;
  else
    mol->work.work_bond.angle = -(PI / 2 - theta);

  if (arced_cycle)
  // cycle with arc
  {
    double radius = ((double)mol->work.work_bond.length) / (2 * sin(theta / 2));
    Vector2 center = VEC2_POS_PLUS_POLAR(mol->work.pos, radius, mol->work.work_bond.angle + (PI / 2 - theta / 2));
    DrawCircleV(center, 2, RED);
    DrawRing(center, radius * .6 - BOND_LINE_THICKNESS / 2, radius * .6 + BOND_LINE_THICKNESS / 2, arc_start_angle, arc_end_angle, 100, mol->info.color);
  }

  if (*mol->work.work_ptr != '(')
  {
    eprintf("Expected '(' as Cycle start character, but got '%c'\n", *mol->work.work_ptr);
    return -1;
  }
  ++(mol->work.work_ptr); // skip said '('

  // printf("Bond has %sbeen drawn\n", mol->info.bond_drawn ? "" : "not ");

  ++mol->info.in_cycle;

  double start_atom_radius = mol->work.work_bond.r_next; // store the width of the first atom for when the cycle meets up at the end

  uint32_t i = 1;
  while (i < n)
  {
    int r = DrawMolStep(mol, (3 * PI / 2 - PI / n), mol->work.work_bond.angle, theta);
    if (r < 0)
      return -1;
    if (r == 1)
    {
      mol->work.work_bond = saved_bond;
      mol->work.pos = saved_pos;
      --(mol->info.in_cycle);
      return 0;
    }

    mol->work.work_bond.angle += theta;
    ++i;
    continue;
  }

  int ret = DrawMolSpecialFeatures(mol, (3 * PI / 2 - PI / n), theta);
  if (ret < 0)
    return -1;
  if (ret == 1)
  {
    mol->work.work_bond = saved_bond;
    mol->work.pos = saved_pos;
    --(mol->info.in_cycle);
    return 0;
  }

  if (IS_BOND_CHAR(*mol->work.work_ptr))
  {
    // this is necessary, as normally DrawMolStep does it but we do not call it here
    mol->work.draw_cursor_should_be_updated = 0;

    if (get_next_bondEx(mol, mol->work.work_bond.angle))
      return -1;
    mol->work.work_bond.r_prev = mol->work.work_bond.r_next;
    mol->work.work_bond.r_next = start_atom_radius;
    DrawBond_UpdatePos(mol);
  }

  mol->work.work_bond = saved_bond;
  mol->work.pos = saved_pos;
  --mol->info.in_cycle;

  if (skip_to_closing_paren(&mol->work.work_ptr))
    return -1;

  if (*mol->work.work_ptr != ')')
  {
    eprintf("Cycle missing closing parentesis, in molecule %s\n", mol->info.text);
    return -1;
  }
  ++(mol->work.work_ptr); // skip said ')'

  return 0;
}

int DrawBranches(mol *mol)
{
  while (*mol->work.work_ptr == '(')
  {
    mol->info.color = BLACK;
    if (mol->work.work_ptr == mol->info.cursor.brptr && mol->info.cursor.mode == MODE_BR_SELECT)
      mol->info.color = GRAY;
    ++(mol->work.work_ptr); // skip said '('

    if (*mol->work.work_ptr == '[')
    {
      if (get_next_angleMol(&mol->work))
        return -1;
      mol->info.base_angle = mol->work.work_bond.angle;
    }

    // double br_angle = theta;
    bond saved_bond = mol->work.work_bond;
    if (get_next_bond(mol))
      return -1;

    char *saved_work_ptr = mol->work.work_ptr;
    if (get_next_atom(mol))
      return -1;
    mol->work.work_ptr = saved_work_ptr;

    mol->work.work_bond.r_prev = mol->work.work_bond.r_next;

    mol->work.work_bond.r_next = 0;
    if (mol->info.atom[0] != '\0')
      mol->work.work_bond.r_next = __max(MeasureAtom(&mol->info), mol->info.font_size) / 2 + BOND_PADDING;

    DrawBond(&mol->info, mol->work.work_bond, mol->work.pos);

    mol->info.color = BLACK;

    double saved_angle = mol->info.base_angle;
    mol->info.base_angle = mol->work.work_bond.angle;
    char *saved_txt = mol->info.text;
    mol->info.text = mol->work.work_ptr;
    Vector2 saved_pos = mol->work.pos;
    mol->work.pos = VEC2_POS_PLUS_POLAR(mol->work.pos, mol->work.work_bond.length, mol->work.work_bond.angle);
    int ret;
    if ((ret = DrawMol_inside(mol)) < 0)
      return -1;
    mol->info.base_angle = saved_angle;
    mol->info.text = saved_txt;
    mol->work.pos = saved_pos;
    mol->work.work_bond = saved_bond;

    if (ret == 1)
      return 0;
  }

  mol->info.color = BLACK;
  return 0;
}

int DrawMol(mol *mol, Vector2 start_pos, double angle, Font font)
{
  mol->info.start_pos = start_pos;
  mol->info.base_angle = angle;
  mol->info.font = font;
  mol->info.bond_drawn = 0;
  mol->info.in_cycle = 0;

  mol->work = (mol_work_data){
      .draw_cursor_should_be_updated = 0,
      .pos = mol->info.start_pos,
      .work_ptr = mol->info.text,
      .work_bond = (bond){
          .angle = angle,
          .length = BOND_LENGTH,
          .type = SINGLE_BOND,
          .r_next = 0,
          .r_prev = 0,
      }};

  mol->info.draw_cursor.width[0] = MeasureTextEx(mol->info.font, mol->info.draw_cursor.text + DRAW_CURSOR_TEXT_LEFT, mol->info.font_size, 0).x;
  mol->info.draw_cursor.width[1] = MeasureTextEx(mol->info.font, mol->info.draw_cursor.text + DRAW_CURSOR_TEXT_RIGHT, mol->info.font_size, 0).x;

  if (DrawMol_inside(mol) < 0)
    return -1;

  if (mol->info.draw_cursor.pos.x != -1 && mol->info.draw_cursor.pos.y != -1)
  {
    DrawTextCodepoint(mol->info.font, mol->info.draw_cursor.text[DRAW_CURSOR_TEXT_LEFT],
                      Vector2Add(mol->info.draw_cursor.pos,
                                 (Vector2){-mol->info.draw_cursor.atom_width / 2 - mol->info.draw_cursor.width[0], -mol->info.font_size / 2}),
                      mol->info.font_size, GRAY); // draw left cursor before atom
    DrawTextCodepoint(mol->info.font, mol->info.draw_cursor.text[DRAW_CURSOR_TEXT_RIGHT],
                      Vector2Add(mol->info.draw_cursor.pos,
                                 (Vector2){mol->info.draw_cursor.atom_width / 2, -mol->info.font_size / 2}),
                      mol->info.font_size, GRAY); // draw right cursor after atom
  }

  return 0;
}

int DrawMol_inside(mol *mol)
{
  if (mol->info.text == NULL || mol == NULL)
  {
    eprintf("Provided mol or mol text pointer was NULL !\n");
    return -1;
  }

  memset(mol->info.atom, 0, ATOM_MAX_LENGTH * sizeof(mol->info.atom[0]));

  mol->work.work_bond = (bond){.angle = mol->info.base_angle, .length = BOND_LENGTH, .type = SINGLE_BOND, .r_prev = 0, .r_next = 0};

  // DrawCircleV(mol->work.pos, 2, RED);

  // check if whole molecule is rotated
  if (*mol->work.work_ptr == '[')
  {
    if (get_next_angleMol(&mol->work))
      return -1;
    mol->info.base_angle = mol->work.work_bond.angle;
  }

  mol->work.draw_cursor_should_be_updated = 0;
  if (mol->work.work_ptr == mol->info.cursor.pos)
  {
    mol->info.draw_cursor.pos = mol->work.pos;
    mol->work.draw_cursor_should_be_updated = 1;
  }

  if (get_next_atom(mol))
    return -1;

  MeasureAtom_InPlace(mol);

  if (DrawAtom(mol))
    return -1;

  int ret;
  if ((ret = DrawMolSpecialFeatures(mol, 0, NaN)) < 0)
    return -1;
  if (ret == 1)
    return 1;

  while (*mol->work.work_ptr != '\0')
  {
    ret = DrawMolStep(mol, 0, mol->info.base_angle, NaN);
    if (ret < 0)
      return -1;
    if (ret == 1) // return value of 1 indicates that this is the end of this branch
      return 0;
  }

  if (mol->work.work_ptr == mol->info.cursor.pos)
  {
    mol->info.draw_cursor.pos = mol->work.pos;
    mol->info.draw_cursor.atom_width = mol->work.atom_w;
  }

  return 0;
}

int DrawMolSpecialFeatures(mol *mol, double branch_angle_offset, double prev_theta)
{
  double saved_angle;
  Vector2 saved_pos;
  switch (*mol->work.work_ptr)
  {
  case '(':
    saved_pos = mol->work.pos;
    saved_angle = mol->info.base_angle;
    mol->info.base_angle = mol->work.work_bond.angle + branch_angle_offset;
    if (DrawBranches(mol))
      return -1;
    mol->info.base_angle = saved_angle;
    mol->work.pos = saved_pos;
    break;

  case ')':
    ++mol->work.work_ptr; // skip said ')'
    return 1;

  case '*':
    if (DrawCycle(mol, prev_theta))
      return -1;
    break;

  default:
    return 2;
  }

  return 0;
}

/*
 * returns 1 upon end of branch
 */
int DrawMolStep(mol *mol, double branch_angle_offset, double bond_default_angle, double prev_theta)
{
  if (*mol->work.work_ptr == ')')
  {
    ++(mol->work.work_ptr);
    return 1;
  }

  if (get_next_bondEx(mol, bond_default_angle))
    return -1;

  mol->work.draw_cursor_should_be_updated = 0;
  if (mol->work.work_ptr == mol->info.cursor.pos)
    mol->work.draw_cursor_should_be_updated = 1;

  if (get_next_atom(mol))
    return -1;

  if (MeasureAtom_InPlace(mol))
    return -1;

  DrawBond_UpdatePos(mol);

  // DrawCircle(posx, posy, 2, RED);

  if (DrawAtom(mol))
    return -1;

  int ret;
  while ((ret = DrawMolSpecialFeatures(mol, branch_angle_offset, prev_theta)) != 2)
  {
    if (ret)
      return ret;
  }

  if (*mol->work.work_ptr == '\0')
    return 0;

  return 0;
}

int create_mol(mol *empty_mol, char *text)
{
  if (text[0] != '\0')
  {
    eprintf("Expected text to start with '\\0' for backtracking, but got '%c'\n", text[0]);
    return -1;
  }

  *empty_mol = (mol){
      .info = (mol_info){
          .text = text + 1,
          .font_size = MOL_BASE_FONT_SIZE,
          .color = BLACK,
          .bond_drawn = 0,
          .draw_cursor = (draw_cursor){
              .pos = (Vector2){-1, -1},
              .text = BASE_DRAW_CURSOR,
          },
          .cursor.pos = text + 1,
      },
  };

  skip_white_space(&empty_mol->info.text);

  if (empty_mol->info.cursor.pos[0] == '[')
    if (skip_next_angle(&empty_mol->info.cursor.pos))
      return -1;

  return 0;
}

void skip_white_space(char **s)
{
  while (**s == ' ' || **s == '\t' || **s == '\n')
    ++(*s);
}
