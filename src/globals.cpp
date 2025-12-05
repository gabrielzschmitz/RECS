#include "globals.h"

const int GAME_W = 640;
const int GAME_H = 360;
// Common GAME_W x GAME_H resolutions and their scaled window sizes:
//
// Base Resolution | SCALE | Scaled Window Size (W x H)
// ----------------------------------------------------
//    320 x 180    |   6   |    1920 x 1080
//    320 x 180    |   3   |     960 x 540
//    320 x 180    |   4   |    1280 x 720
//    640 x 360    |   2   |    1280 x 720
//   1280 x 720    |   1   |    1280 x 720
const int SCALE = 2;
Font defaultFont;

const int ACTIVE_W = 639;
const int ACTIVE_H = 359;
std::vector<Entity> gridEntities; // size ACTIVE_W * ACTIVE_H
std::vector<bool> currentState;   // size ACTIVE_W * ACTIVE_H
std::vector<bool> nextState;      // size ACTIVE_W * ACTIVE_H
