#include "draw.h"
#include "raylib.h"
#include <cmath>

void DrawBoundingBox(Rectangle rect, float thickness, float padding,
                     Color color) {
  // Apply padding
  float rx = rect.x - padding;
  float ry = rect.y - padding;
  float rw = rect.width + padding * 2;
  float rh = rect.height + padding * 2;

  // Expand outward by half the stroke thickness
  float half = thickness * 0.5f;
  rx -= half;
  ry -= half;
  rw += thickness;
  rh += thickness;

  // Pixel-align the bounding box
  int ix = (int)floorf(rx);
  int iy = (int)floorf(ry);
  int iw = (int)ceilf(rx + rw) - ix;
  int ih = (int)ceilf(ry + rh) - iy;

  DrawRectangleLinesEx(Rectangle{(float)ix, (float)iy, (float)iw, (float)ih},
                       thickness, color);
}
