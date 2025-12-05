#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>

struct PlayerTag {};

struct TransformComponent {
  Vector2 coords;
  float scale = 1.0f;
};

namespace Directions {
static const Vector2 UP = {0.0f, -1.0f};
static const Vector2 DOWN = {0.0f, 1.0f};
static const Vector2 LEFT = {-1.0f, 0.0f};
static const Vector2 RIGHT = {1.0f, 0.0f};
} // namespace Directions

struct DirectionComponent {
  Vector2 dir = Directions::RIGHT;
};

struct SpriteComponent {
  Texture2D src;
  Vector2 origin;
  Vector2 size;
};

struct AnimationFrame {
  Vector2 origin;
  Vector2 size;
  float duration;
  float elapsed;
};

struct AnimationComponent {
  int currentFrame;
  std::string selectedAnimation;
  std::unordered_map<std::string, std::vector<AnimationFrame>> animations;
};

struct BoundingBoxComponent {
  Rectangle rect;
  float thickness;
  float padding;
  Color color;

  // Constructor for convenience
  BoundingBoxComponent(Rectangle r = {0, 0, 0, 0}, float t = 1.0f,
                       float p = 0.0f, Color c = RED)
      : rect(r), thickness(t), padding(p), color(c) {}
};
