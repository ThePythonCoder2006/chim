#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "raylib.h"
#include "raymath.h"

#include "mol.h"
#include "cursor.h"
#include "log.h"

#define SWIDTH 1400
#define SHEIGHT 800

#define FIRA_CODE_FONT_PATH "fonts/Fira_Code/static/FiraCode-Regular.ttf"

char g_atom_buffer[ATOM_MAX_LENGTH] = {0};

int main(int argc, char **argv)
{
  (void)argc, (void)argv;

  mol mol = {.info.text = NULL};
  int r = -1; // if I forgot to uncomment a mol, this will error out by default

  // r = create_mol(&mol, "\0HO-[:90](=[:150]O)-[:30]N(-[:90]Ph)-[::-60](-[:-90]NO2)-[::60](-[:90]C6H5)-[::-60](=[:30]O)-[:-90]OH");
  r = create_mol(&mol, "\0[:20]NO2-[:60](-[:120]Cl)=[:0](-[:60]Cl)-[:-60]");
  // r = create_mol(&mol, "\0(-[:90]Cl)(-[:-120]Cl)(<[:-60]Cl)(<:[:-30]Cl)-");
  // r = create_mol(&mol, "\0A-B(-[:45]W-X)-C");
  // r = create_mol(&mol, "\0[:75]R-C(=[::+60]O)-[::-60]O-[::-60]C(=[::+60]O)-[::-60]R'");
  // r = create_mol(&mol, "\0-[:0]{C6H10O6}10000-[:0]");
  // r = create_mol(&mol, "\0(-[:-150]X*6(-=-=-=))-[:-30](<[:-90]CH3)-[:30]N-[:-30]CH3");
  // r = create_mol(&mol, "\0H-C~N");
  // r = create_mol(&mol, "\0HC~CH");
  // r = create_mol(&mol, "\0-**5(---)-[:90]");
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
  // r = create_mol(&mol, "\0N(-[:30]O)(=[:150]O)-[:-90]*5(=N-N=N-NH-)");

  if (r)
  {
    eprintf("Creating molecule object failed or you forgot to uncomment a line\n\t" COLORED_INFO " mol.info.text = %s\n", mol.info.text);
    return -1;
  }

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(SWIDTH, SHEIGHT, "Chim");
  SetTargetFPS(60);

  Font Fira = LoadFont(FIRA_CODE_FONT_PATH);

  uint8_t quit = 0;
  while (!WindowShouldClose() && !quit)
  {
    int keycode;

    while ((keycode = GetKeyPressed()) != 0)
    {
      if (keycode == KEY_A)
      {
        quit = 1;
        break;
      }

      if (handle_key_press(keycode, &mol.info.cursor))
      {
        eprintf("Handling pressed keys has failed !!\n");
        return 1;
      }
    }

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
