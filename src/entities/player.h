// player.h
#pragma once

#include "../engine/components.h"
#include "../engine/ecs.h"
#include "raylib.h"

extern const int GAME_W;
extern const int GAME_H;

inline Entity CreatePlayer(ECS &ecs) {
  const int PLAYER_SCALE = 2;
  Entity player = ecs.create_entity();

  // Player tag
  ecs.add<PlayerTag>(player, PlayerTag{});

  // Load texture
  Texture2D spriteSheet = LoadTexture("sprites/char.png");
  SetTextureFilter(spriteSheet, TEXTURE_FILTER_POINT);

  // Sprite Component
  SpriteComponent spr{};
  spr.src = spriteSheet;
  spr.origin = {0, 0};
  spr.size = {16, 16};
  ecs.add<SpriteComponent>(player, spr);

  // Animation Component
  AnimationComponent animComp{};
  animComp.currentFrame = 0;
  animComp.selectedAnimation = "idle";

  // Define idle animation frames
  std::vector<AnimationFrame> idleFrames;
  for (int i = 0; i < 20; i++) {
    AnimationFrame frame;
    frame.origin = {float(i * 16), 0};
    frame.size = {16, 16};
    frame.duration = 0.10f;
    frame.elapsed = 0.0f;
    idleFrames.push_back(frame);
  }
  animComp.animations["idle"] = idleFrames;

  // Define running animation frames
  std::vector<AnimationFrame> runningFrames;
  for (int i = 0; i < 12; i++) {
    AnimationFrame frame;
    frame.origin = {float(i * 16), 16};
    frame.size = {16, 16};
    frame.duration = 0.05f;
    frame.elapsed = 0.0f;
    runningFrames.push_back(frame);
  }
  animComp.animations["running"] = runningFrames;

  ecs.add<AnimationComponent>(player, animComp);

  // Transform centered in game world
  TransformComponent trans{};
  trans.scale = PLAYER_SCALE;
  trans.coords.x = (spr.size.x * PLAYER_SCALE) / 2.0f;
  trans.coords.y = (spr.size.y * PLAYER_SCALE) / 2.0f;
  ecs.add<TransformComponent>(player, trans);
  ecs.add<DirectionComponent>(player, DirectionComponent{});

  // Bounding Box
  Rectangle playerBoxRect = {trans.coords.x, trans.coords.y,
                             spr.size.x * trans.scale,
                             spr.size.y * trans.scale};
  BoundingBoxComponent bbox(playerBoxRect, 1.0f, 0.0f, BLUE);
#ifdef DEBUG
  ecs.add<BoundingBoxComponent>(player, bbox);
#endif // DEBUG

  return player;
}
