#pragma once
#include "../engine/ecs.h"
#include "ecs_sample_components.h"
#include "test_lib.h"

#include <cassert>
#include <random>
#include <vector>

// ------------------------------------------------------------
// BENCHMARKS
// ------------------------------------------------------------
BENCH(bench_ecs_add_remove_cycles) {
  ECS ecs;
  const size_t N = 300000;
  std::vector<Entity> ents;
  ents.reserve(N);

  for (size_t i = 0; i < N; i++)
    ents.push_back(ecs.create_entity());

  for (size_t i = 0; i < N; i++)
    ecs.add<Position>(ents[i], (float)i, (float)i * 2);

  for (size_t i = 0; i < N; i += 2)
    ecs.remove<Position>(ents[i]);

  for (size_t i = 0; i < N; i++)
    ecs.add<Position>(ents[i], (float)i, (float)i * 3);
}

BENCH(bench_ecs_destroy_reuse) {
  ECS ecs;
  const size_t N = 500000;

  std::vector<Entity> ents(N);
  for (size_t i = 0; i < N; i++)
    ents[i] = ecs.create_entity();

  for (size_t i = 0; i < N; i += 2)
    ecs.destroy_entity(ents[i]);

  for (size_t i = 0; i < N / 2; i++)
    ecs.create_entity();
}

BENCH(bench_ecs_world_sim) {
  ECS ecs;
  const size_t N = 200000;

  std::vector<Entity> ents(N);
  for (size_t i = 0; i < N; i++)
    ents[i] = ecs.create_entity();

  std::mt19937 rng(999);
  std::uniform_int_distribution<int> dist(0, N - 1);

  for (int frame = 0; frame < 2000; frame++) {
    int e = dist(rng);
    if (frame % 3 == 0)
      ecs.add<Velocity>(ents[e], 1.f, 2.f);
    else if (frame % 4 == 0 && ecs.has<Velocity>(ents[e]))
      ecs.remove<Velocity>(ents[e]);
    else if (ecs.has<Position>(ents[e]))
      ecs.get<Position>(ents[e]).x++;
  }
}

BENCH(bench_ecs_multi_component_view) {
  ECS ecs;
  const size_t N = 400000;

  std::vector<Entity> ents(N);
  for (size_t i = 0; i < N; i++) {
    ents[i] = ecs.create_entity();
    ecs.add<Position>(ents[i], (float)i, (float)i);
    if (i % 2 == 0)
      ecs.add<Velocity>(ents[i], 1.f, 1.f);
    if (i % 3 == 0)
      ecs.add<Health>(ents[i], 100);
  }

  ecs.view<Position, Velocity, Health>(
      [&](Entity, Position &p, Velocity &v, Health &h) {
        p.x += v.vx;
        h.hp -= 1;
      });
}

BENCH(bench_ecs_cache_random) {
  ECS ecs;
  const size_t N = 600000;

  std::vector<Entity> ents(N);
  for (size_t i = 0; i < N; i++) {
    ents[i] = ecs.create_entity();
    ecs.add<Position>(ents[i], (float)i, (float)i);
  }

  std::mt19937 rng(321);
  std::shuffle(ents.begin(), ents.end(), rng);

  for (Entity e : ents) {
    if (ecs.has<Position>(e))
      ecs.get<Position>(e).x++;
  }
}
