#pragma once
#include "../engine/ecs.h"
#include "ecs_sample_components.h"
#include "test_lib.h"

#include <cassert>
#include <random>
#include <vector>

// ------------------------------------------------------------
// TESTS
// ------------------------------------------------------------
TEST(test_ecs_basic) {
  ECS ecs;

  Entity e1 = ecs.create_entity();
  Entity e2 = ecs.create_entity();
  Entity e3 = ecs.create_entity();

  ecs.add<Position>(e1, 1.f, 2.f);
  ecs.add<Velocity>(e1, 0.1f, 0.2f);

  ecs.add<Position>(e2, 10.f, 20.f);

  ecs.add<Position>(e3, -1.f, -2.f);
  ecs.add<Velocity>(e3, 5.f, 6.f);
  ecs.add<Health>(e3, 50);

  assert(ecs.has<Position>(e1));
  assert(!ecs.has<Health>(e1));

  assert(ecs.get<Position>(e1).x == 1);
  assert(ecs.get<Velocity>(e3).vx == 5);

  ecs.remove<Velocity>(e1);
  assert(!ecs.has<Velocity>(e1));

  ecs.destroy_entity(e2);
  assert(!ecs.is_alive(e2));
}

TEST(test_ecs_view) {
  ECS ecs;

  Entity a = ecs.create_entity();
  Entity b = ecs.create_entity();
  Entity c = ecs.create_entity();
  Entity d = ecs.create_entity();

  ecs.add<Position>(a, 1.f, 1.f);
  ecs.add<Velocity>(a, 2.f, 2.f);

  ecs.add<Position>(b, 10.f, 10.f);

  ecs.add<Velocity>(c, -1.f, -1.f);
  ecs.add<Position>(d, 0.f, 0.f);
  ecs.add<Velocity>(d, 4.f, 4.f);

  int count_pos = 0;
  ecs.view<Position>([&](Entity, Position &) { count_pos++; });
  assert(count_pos == 3);

  int count_pos_vel = 0;
  ecs.view<Position, Velocity>(
      [&](Entity, Position &, Velocity &) { count_pos_vel++; });
  assert(count_pos_vel == 2);
}

TEST(test_ecs_random_access) {
  ECS ecs;
  const int N = 200000;
  std::vector<Entity> ents(N);

  for (int i = 0; i < N; i++)
    ents[i] = ecs.create_entity();

  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> dist(0, N - 1);

  for (int i = 0; i < N; i++) {
    int idx = dist(rng);
    ecs.add<Position>(ents[idx], float(i), float(i * 2));
  }

  for (int i = 0; i < N / 3; i++) {
    int idx = dist(rng);
    ecs.remove<Position>(ents[idx]);
  }

  int alive_count = 0;
  ecs.view<Position>([&](Entity, Position &) { alive_count++; });

  assert(alive_count > 0);
}
