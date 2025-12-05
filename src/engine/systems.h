#pragma once
#include "../entities/conway.h"
#include "components.h"
#include "ecs.h"
#include "raylib.h"

inline void SimulateConway(ECS &ecs) {
  for (int y = 0; y < ACTIVE_H; ++y) {
    for (int x = 0; x < ACTIVE_W; ++x) {
      int liveNeighbors = 0;

      for (auto &offset : neighbor_offsets) {
        int nx = x + static_cast<int>(offset.x);
        int ny = y + static_cast<int>(offset.y);
        if (nx >= 0 && nx < ACTIVE_W && ny >= 0 && ny < ACTIVE_H)
          if (currentState[index(nx, ny)])
            liveNeighbors++;
      }

      bool alive = currentState[index(x, y)];
      bool nextAlive = false;

      if (alive)
        nextAlive = (liveNeighbors == 2 || liveNeighbors == 3);
      else
        nextAlive = (liveNeighbors == 3);

      nextState[index(x, y)] = nextAlive;
    }
  }

  for (int i = 0; i < (int)gridEntities.size(); ++i) {
    CellComponent &cell = ecs.get<CellComponent>(gridEntities[i]);
    cell.color = nextState[i] ? WHITE : BLACK;
  }

  currentState.swap(nextState);
}

inline void RenderCells(ECS &ecs) {
  ecs.view<CellComponent>([&](Entity e, CellComponent &quad) {
    if (is_alive(quad.color))
      DrawRectangleRec(quad.rect, quad.color);
  });
}
