#pragma once
#include "../draw.h"
#include "components.h"
#include "ecs.h"
#include "raylib.h"
#include <cmath>

inline void RenderSprites(ECS &ecs, float dt) {
  ecs.view<SpriteComponent>([&](Entity e, SpriteComponent &s) {
    TransformComponent &t = ecs.get<TransformComponent>(e);

    Rectangle srcRect = {0, 0, s.size.x, s.size.y};
    Vector2 origin = s.origin;

    if (ecs.has<AnimationComponent>(e)) {
      AnimationComponent &anim = ecs.get<AnimationComponent>(e);
      auto &frames = anim.animations[anim.selectedAnimation];
      AnimationFrame &frame = frames[anim.currentFrame];

      frame.elapsed += dt;
      if (frame.elapsed >= frame.duration) {
        frame.elapsed -= frame.duration;
        anim.currentFrame = (anim.currentFrame + 1) % frames.size();
      }

      srcRect = {frame.origin.x, frame.origin.y, frame.size.x, frame.size.y};
      origin = {0, 0};
    }

    if (ecs.has<DirectionComponent>(e)) {
      DirectionComponent &dir = ecs.get<DirectionComponent>(e);
      if (dir.dir.x == Directions::LEFT.x)
        srcRect.width = -srcRect.width;
    }

    Rectangle dstRect = {t.coords.x, t.coords.y, abs(srcRect.width) * t.scale,
                         srcRect.height * t.scale};
    DrawTexturePro(s.src, srcRect, dstRect, origin, 0, WHITE);
  });
}

inline void MovePlayers(ECS &ecs, float dt, float moveSpeed) {
  ecs.view<PlayerTag, TransformComponent, DirectionComponent>(
      [&](Entity e, PlayerTag &, TransformComponent &t,
          DirectionComponent &dirComp) {
        bool isMoving = false;
        Vector2 movement = {0, 0};

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
          movement.y -= 1;
          isMoving = true;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
          movement.y += 1;
          isMoving = true;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
          movement.x -= 1;
          isMoving = true;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
          movement.x += 1;
          isMoving = true;
        }

        if (isMoving) {
          float length =
              sqrt(movement.x * movement.x + movement.y * movement.y);
          if (length != 0) {
            movement.x /= length;
            movement.y /= length;
          }
          t.coords.x += movement.x * moveSpeed * dt;
          t.coords.y += movement.y * moveSpeed * dt;

          if (length > 0.0f)
            dirComp.dir = movement;
        }

        // Switch animation state based on movement
        if (ecs.has<AnimationComponent>(e)) {
          AnimationComponent &anim = ecs.get<AnimationComponent>(e);

          std::string newAnim = isMoving ? "running" : "idle";

          if (anim.selectedAnimation != newAnim) {
            anim.selectedAnimation = newAnim;
            anim.currentFrame = 0;

            auto &frames = anim.animations[anim.selectedAnimation];
            for (auto &frame : frames)
              frame.elapsed = 0.0f;
          }
        }
      });
}

inline void DrawBoundingBoxes(ECS &ecs) {
  ecs.view<BoundingBoxComponent, TransformComponent>(
      [&](Entity e, BoundingBoxComponent &bbox, TransformComponent &t) {
        bbox.rect.x = t.coords.x;
        bbox.rect.y = t.coords.y;
        DrawBoundingBox(bbox.rect, bbox.thickness, bbox.padding, bbox.color);
      });
}
