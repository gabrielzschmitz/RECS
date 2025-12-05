// main.cpp
#include "engine/ecs.h"
#include "engine/systems.h"
#include "entities/conway.h"
#include "globals.h"
#include "raylib.h"
#include "resource_dir.h"

int main() {
  int W = GAME_W * SCALE;
  int H = GAME_H * SCALE;

  InitWindow(W, H, "GAME");

  SearchAndSetResourceDir("resources");

  // Render (pixel-perfect)
  Camera2D camera = {0};
  camera.target = (Vector2){(float)GAME_W / 2, (float)GAME_H / 2};
  camera.offset = (Vector2){(float)W / 2, (float)H / 2};
  camera.zoom = SCALE;
  defaultFont = LoadFont("fonts/simple-font.png");

  // ECS
  ECS ecs;
  CreateConway(ecs);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // UPDATE
    SimulateConway(ecs);

    // DRAW
    BeginDrawing();
    ClearBackground((Color){20, 22, 34, 255});

    BeginMode2D(camera);
    RenderCells(ecs);

    EndMode2D();
    DrawTextEx(defaultFont, TextFormat("FPS: %d", GetFPS()), (Vector2){10, 10},
               defaultFont.baseSize * 2, 1, (Color){255, 80, 150, 255});
    EndDrawing();
  }

  // Cleanup
  UnloadFont(defaultFont);
  CloseWindow();
  return 0;
}
