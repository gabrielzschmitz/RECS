#pragma once
#include "engine/ecs.h" // your ECS and Entity definitions
#include "raylib.h"     // for Color, Rectangle, etc.
#include <vector>

extern const int GAME_W;
extern const int GAME_H;
extern const int SCALE;
extern Font defaultFont;

extern const int ACTIVE_W;
extern const int ACTIVE_H;
extern std::vector<Entity> gridEntities; // size ACTIVE_W * ACTIVE_H
extern std::vector<bool> currentState;   // size ACTIVE_W * ACTIVE_H
extern std::vector<bool> nextState;      // size ACTIVE_W * ACTIVE_H
