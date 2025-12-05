#pragma once

#include "../engine/components.h"
#include "../engine/ecs.h"
#include "../globals.h"
#include "raylib.h"
#include <random>

// Helper to get neighbors coordinates offsets
inline const std::vector<Vector2> neighbor_offsets = {
    {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

// Helper: Check if a color is "alive" (white)
inline bool is_alive(const Color &c) {
  return c.r > 127 && c.g > 127 && c.b > 127;
}

// Helper to convert 2D coords to index
inline int index(int x, int y) { return x + y * ACTIVE_W; }

inline void CreateConway(ECS &ecs) {
  gridEntities.resize(ACTIVE_W * ACTIVE_H);
  currentState.resize(ACTIVE_W * ACTIVE_H, false);
  nextState.resize(ACTIVE_W * ACTIVE_H, false);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> distProb(0.0, 1.0);
  const float aliveProbability = 0.65f;

  for (int x = 0; x < ACTIVE_W; ++x) {
    for (int y = 0; y < ACTIVE_H; ++y) {
      int i = x + y * ACTIVE_W;
      Entity e = ecs.create_entity();
      bool alive = distProb(gen) < aliveProbability;
      Color cellColor = alive ? WHITE : BLACK;
      ecs.add<CellComponent>(e, Rectangle{(float)(x), (float)(y), 1.f, 1.f},
                             cellColor);
      gridEntities[i] = e;
      currentState[i] = alive;
    }
  }
}
