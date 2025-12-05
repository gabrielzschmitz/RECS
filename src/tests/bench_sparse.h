#pragma once
#include <algorithm>
#include <cassert>
#include <random>
#include <vector>

#include "../engine/sparse_set.h"
#include "test_lib.h"

// Test count
constexpr int N = 10'000'000;

// ------------------------------------------------------------
// BENCHMARKS
// ------------------------------------------------------------
BENCH(bench_sparse_insert) {
  std::vector<uint32_t> keys(N);
  for (int i = 0; i < N; i++)
    keys[i] = i;

  std::mt19937 rng(123);
  std::shuffle(keys.begin(), keys.end(), rng);

  SparseSet<uint32_t, uint32_t> s;
  for (uint32_t k : keys)
    s.insert(k, k + 1);
}

BENCH(bench_sparse_lookup) {
  SparseSet<uint32_t, uint32_t> s;
  for (int i = 0; i < N; i++)
    s.insert(i, i + 1);

  std::vector<uint32_t> keys(N);
  for (int i = 0; i < N; i++)
    keys[i] = i;

  std::mt19937 rng(123);
  std::shuffle(keys.begin(), keys.end(), rng);

  uint64_t sum = 0;
  for (uint32_t k : keys)
    sum += s.get(k);

  volatile uint64_t sink = sum;
  (void)sink;
}

BENCH(bench_sparse_iteration) {
  SparseSet<uint32_t, uint32_t> s;
  for (int i = 0; i < N; i++)
    s.insert(i, i + 1);

  uint64_t sum = 0;
  s.for_each([&](uint32_t key, uint32_t &v) { sum += v; });

  volatile uint64_t sink = sum;
  (void)sink;
}

BENCH(bench_sparse_erase) {
  std::vector<uint32_t> keys(N);
  for (int i = 0; i < N; i++)
    keys[i] = i;

  std::mt19937 rng(123);
  std::shuffle(keys.begin(), keys.end(), rng);

  SparseSet<uint32_t, uint32_t> s;
  for (uint32_t k : keys)
    s.insert(k, k + 1);

  for (uint32_t k : keys)
    s.erase(k);
}

BENCH(bench_sparse_random_insert_erase) {
  constexpr int OPS = N / 2;

  std::vector<uint32_t> keys(N);
  for (int i = 0; i < N; i++)
    keys[i] = i;

  std::mt19937 rng(123);
  std::shuffle(keys.begin(), keys.end(), rng);

  SparseSet<uint32_t, uint32_t> s;

  for (int i = 0; i < OPS; i++)
    s.insert(keys[i], i);

  for (int i = 0; i < OPS; i++)
    s.erase(keys[i]);
}
