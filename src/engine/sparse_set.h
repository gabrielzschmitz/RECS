#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <vector>

/**
 * ======================================================================
 * SparseSet<Entity, T>
 * ======================================================================
 *
 * A high-performance associative structure mapping:
 *
 *      Entity ID  ->  Component T
 *
 * Implemented using:
 *
 *   • A *sparse* array (paged, lazily allocated)
 *   • A *dense* array (packed, swap-remove)
 *
 * The SparseSet provides:
 *   • O(1) insert
 *   • O(1) contains
 *   • O(1) erase (swap with last)
 *   • O(1) get
 *   • tight packed iteration over all components
 *
 * This is equivalent to what EnTT and Flecs use internally.
 *
 * Memory Layout
 * ----------------------------------------------------------------------
 *   sparse[e] = index into dense arrays   (or INVALID if empty)
 *
 *   dense_entities = [e0,  e1,  e2,  ...]
 *   components      = [c0,  c1,  c2,  ...]  // c[i] belongs to e[i]
 *
 * When erasing, the last element is moved into the removed slot:
 *
 *     [eA, eB, eC, eD]         erase(eB)
 *     [eA, eD, eC]             (eD moved into index 1)
 *
 * Paged sparse storage avoids allocating a 2-GB sparse array for worlds
 * with large random entity IDs. Only touched pages are allocated.
 *
 * ======================================================================
 */
template <typename T, typename Entity = uint32_t> class SparseSet {
public:
  static_assert(
      !std::is_void_v<T>,
      "SparseSet<T=void> is not allowed. Use a tag component instead.");

  // Value stored in sparse table when element is not present.
  static constexpr Entity INVALID = static_cast<Entity>(-1);

  SparseSet() = default;

  // Frees all sparse pages on destruction.
  ~SparseSet() {
    for (auto *p : pages)
      std::free(p);
  }

  // --------------------------------------------------------------------
  // Paging constants (Sparse array paging)
  // --------------------------------------------------------------------
  // Each page stores 2048 entries of `Entity`.
  static constexpr size_t SPARSE_SET_PAGE_BITS = 11; // log2(2048)
  static constexpr size_t SPARSE_SET_PAGE_SIZE = 1ull << SPARSE_SET_PAGE_BITS;
  static constexpr size_t SPARSE_SET_PAGE_MASK = SPARSE_SET_PAGE_SIZE - 1;

private:
  // ==================================================================
  // Sparse Helpers (Paged sparse table)
  // ==================================================================

  /**
   * Ensure a sparse page exists for a given page index.
   * Allocates a 2048-entry page initialized to INVALID.
   */
  void ensure_page(size_t page_idx) {
    if (page_idx >= pages.size())
      pages.resize(page_idx + 1, nullptr);

    if (!pages[page_idx]) {
      Entity *page =
          (Entity *)std::malloc(sizeof(Entity) * SPARSE_SET_PAGE_SIZE);
      for (size_t i = 0; i < SPARSE_SET_PAGE_SIZE; i++)
        page[i] = INVALID;
      pages[page_idx] = page;
    }
  }

  /**
   * Returns a mutable reference to sparse[e],
   * allocating the page if necessary.
   */
  Entity &sparse_ref(Entity e) {
    const size_t page_idx = e >> SPARSE_SET_PAGE_BITS;
    const size_t offset = e & SPARSE_SET_PAGE_MASK;

    ensure_page(page_idx);
    return pages[page_idx][offset];
  }

  /**
   * Returns a pointer to sparse[e] or nullptr if the page
   * was never allocated.
   *
   * Used by contains().
   */
  const Entity *sparse_ptr(Entity e) const {
    const size_t page_idx = e >> SPARSE_SET_PAGE_BITS;
    if (page_idx >= pages.size() || !pages[page_idx])
      return nullptr;

    return &pages[page_idx][e & SPARSE_SET_PAGE_MASK];
  }

public:
  // ==================================================================
  // Public API
  // ==================================================================

  /**
   * Check whether entity e exists in the set.
   * Complexity: O(1)
   */
  bool contains(Entity e) const {
    const Entity *p = sparse_ptr(e);
    if (!p)
      return false;

    Entity idx = *p;
    // must match dense entity; prevents false positives
    return idx != INVALID && dense_entities[idx] == e;
  }

  /**
   * Insert or update a component for entity e.
   * Returns reference to the stored component.
   *
   * If e already exists, its component is overwritten.
   *
   * Complexity:
   *   Insert empty → O(1)
   *   Overwrite    → O(1)
   */
  T &insert(Entity e, const T &value = T()) {
    if (contains(e)) {
      // Overwrite existing component
      Entity idx = sparse_ref(e);
      components[idx] = value;
      return components[idx];
    }

    // New entity: append to dense arrays
    Entity &slot = sparse_ref(e);
    slot = static_cast<Entity>(dense_entities.size());

    dense_entities.push_back(e);
    components.push_back(value);

    return components.back();
  }

  /**
   * Erase entity e from the set.
   * Preserves packed iteration order by swap-removing.
   *
   * Complexity: O(1)
   */
  void erase(Entity e) {
    if (!contains(e))
      return;

    Entity idx = sparse_ref(e);
    Entity last_idx = static_cast<Entity>(dense_entities.size() - 1);
    Entity last_entity = dense_entities[last_idx];

    // Move last into removed slot
    dense_entities[idx] = last_entity;
    components[idx] = components[last_idx];

    // Update sparse entry for swapped element
    sparse_ref(last_entity) = idx;

    // Remove last
    dense_entities.pop_back();
    components.pop_back();

    // Invalidate removed sparse entry
    sparse_ref(e) = INVALID;
  }

  /**
   * Access existing component.
   * Asserts if e does not exist.
   * Complexity: O(1)
   */
  T &get(Entity e) {
    assert(contains(e));
    return components[sparse_ref(e)];
  }
  const T &get(Entity e) const {
    assert(contains(e));
    return components[sparse_ref(e)];
  }

  // Convenience: set[e] == get(e)
  T &operator[](Entity e) { return get(e); }
  const T &operator[](Entity e) const { return get(e); }

  // ==================================================================
  // Iteration and stats
  // ==================================================================
  //
  /**
   * Iterate over all (entity, component) pairs.
   * Calls f(entity, component_reference).
   */
  template <typename Func> void for_each(Func &&f) {
    size_t n = dense_entities.size();
    for (size_t i = 0; i < n; i++) {
      f(dense_entities[i], components[i]);
    }
  }
  template <typename Func> void for_each(Func &&f) const {
    size_t n = dense_entities.size();
    for (size_t i = 0; i < n; i++) {
      f(dense_entities[i], components[i]);
    }
  }

  // Number of stored components
  size_t size() const { return dense_entities.size(); }

  // Dense list of entity IDs
  const std::vector<Entity> &entities() const { return dense_entities; }

  // Dense component storage
  std::vector<T> &data() { return components; }
  const std::vector<T> &data() const { return components; }

private:
  // ==================================================================
  // Internal storage
  // ==================================================================

  // Sparse paged storage:
  // pages[p][i] = dense index for entity = (p << bits) | i
  std::vector<Entity *> pages;

  // Dense arrays
  std::vector<Entity> dense_entities; //< packed list of entity IDs
  std::vector<T> components;          //< packed component storage
};
