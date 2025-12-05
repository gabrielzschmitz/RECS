#pragma once
#include "raylib.h"

struct CellComponent {
  Rectangle rect;
  Color color;

  CellComponent(const Rectangle &r = {0, 0, 0, 0}, Color c = WHITE)
      : rect(r), color(c) {}
};
