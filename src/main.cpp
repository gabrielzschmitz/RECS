// main.cpp
#include "engine/ecs.h"
#include "engine/systems.h"
#include "entities/player.h"
#include "raylib.h"
#include "resource_dir.h"

const int GAME_W = 320;
const int GAME_H = 180;
const int SCALE = 4;

int main() {
  int W = GAME_W * SCALE;
  int H = GAME_H * SCALE;

  InitWindow(W, H, "GAME");
  SetTargetFPS(240);

  SearchAndSetResourceDir("resources");

  // Render texture (pixel-perfect)
  RenderTexture2D canvas = LoadRenderTexture(GAME_W, GAME_H);
  SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);

  // ECS
  ECS ecs;
  Entity player = CreatePlayer(ecs);

  const float moveSpeed = 85.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // UPDATE
    MovePlayers(ecs, dt, moveSpeed);

    // DRAW
    BeginDrawing();
    BeginTextureMode(canvas);
    ClearBackground(BLANK);

    ClearBackground(BLACK);
    DrawBoundingBoxes(ecs);
    RenderSprites(ecs, dt);

    EndTextureMode();
    DrawTexturePro(canvas.texture, {0, 0, (float)GAME_W, -(float)GAME_H},
                   {0, 0, (float)W, (float)H}, {0, 0}, 0.0f, WHITE);
    EndDrawing();
  }

  // Cleanup
  UnloadRenderTexture(canvas);
  CloseWindow();
  return 0;
}
