// g++ -std=c++17 -O3 -DRUN_TESTS -march=native -o tests tests.cpp && ./tests
#include "test_lib.h"

#include "bench_ecs.h"
#include "bench_sparse.h"
#include "test_ecs.h"
#include "test_sparse.h"

#ifdef RUN_TESTS
int main() {
  TestRegistry::instance().run_all();

  int runs = 20, warmup = 5;
  BenchRegistry::instance().run_all(runs, warmup);
  std::cout << "\n";
}
#endif
