#include "raylib.h"

#define SWIDTH 800
#define SHEIGHT 600

int main()
{
  InitWindow(SWIDTH, SHEIGHT, "Raylib basic window");
  SetTargetFPS(60);
  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("It works!", 20, 20, 20, BLACK);
    DrawLineEx((Vector2){60, 60}, (Vector2){400, 400}, 2.5, BLACK);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}