#pragma once
#include "sparse_set.h" // your SparseSet<T> implementation
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

//
// ECS with component-type IDs + per-entity bitmasks + groups
//
// Design notes:
// - Each component type gets a compact component-id (size_t).
// - Masks are stored as flat blocks of uint64_t per entity (maskBlocks).
// - When component types are added, masks/g roups are resized to accommodate.
// - view<Ts...> iterates the smallest component storage for best perf and
//   uses a mask check (bitwise) to skip non-matching entities quickly.
// - Groups are simply precomputed masks for a set of components.
//

// -------------------------------------------------------------
// Entity type (index + version)
// -------------------------------------------------------------
struct Entity {
  uint32_t index;
  uint32_t version;

  bool operator==(const Entity &other) const {
    return index == other.index && version == other.version;
  }
};

static const Entity INVALID_ENTITY = {UINT32_MAX, UINT32_MAX};

// -------------------------------------------------------------
// Bitset utilities (flat uint64_t blocks)
// -------------------------------------------------------------
class BitMaskHelper {
public:
  // number of bits per block
  static constexpr size_t BLOCK_BITS = 64;

  // helpers
  static size_t blocks_for_bits(size_t bits) {
    return (bits + BLOCK_BITS - 1) / BLOCK_BITS;
  }

  // set nth bit in mask blocks (mask points to first block)
  static inline void set_bit(uint64_t *mask, size_t bit) {
    mask[bit / BLOCK_BITS] |= (uint64_t(1) << (bit % BLOCK_BITS));
  }

  static inline void reset_bit(uint64_t *mask, size_t bit) {
    mask[bit / BLOCK_BITS] &= ~(uint64_t(1) << (bit % BLOCK_BITS));
  }

  static inline bool test_bit(const uint64_t *mask, size_t bit) {
    return (mask[bit / BLOCK_BITS] >> (bit % BLOCK_BITS)) & 1u;
  }

  // returns true if (entity_mask & required_mask) == required_mask
  // both masks must have same number of blocks
  static inline bool test_mask_match(const uint64_t *entity_mask,
                                     const uint64_t *required_mask,
                                     size_t blocks) {
    for (size_t i = 0; i < blocks; ++i) {
      // if any required bit isn't present in entity_mask -> fail
      if ((entity_mask[i] & required_mask[i]) != required_mask[i])
        return false;
    }
    return true;
  }
};

// -------------------------------------------------------------
// ECS class (component id bookkeeping + per-entity masks)
// -------------------------------------------------------------
class ECS {
public:
  ECS() : component_count(0), mask_blocks(0) {}

  // -------------------------------------------
  // Entity management
  // -------------------------------------------
  Entity create_entity() {
    if (!free_list.empty()) {
      uint32_t idx = free_list.back();
      free_list.pop_back();
      // ensure masks exist for this entity index
      ensure_entity_mask_size(idx + 1);
      return {idx, versions[idx]};
    }

    uint32_t idx = static_cast<uint32_t>(versions.size());
    versions.push_back(1);
    // expand masks for the new entity
    ensure_entity_mask_size(versions.size());
    return {idx, 1};
  }

  void destroy_entity(Entity e) {
    if (!is_alive(e))
      return;

    // increment version to invalidate old handles
    versions[e.index]++;

    // remove entity from all component storages and clear mask bits
    for (auto &kv : component_storages) {
      kv.second->erase_entity(e.index);
    }

    // clear mask blocks for this entity
    if (mask_blocks > 0) {
      uint64_t *mask_ptr =
          &entity_masks[static_cast<size_t>(e.index) * mask_blocks];
      for (size_t i = 0; i < mask_blocks; ++i)
        mask_ptr[i] = 0;
    }

    free_list.push_back(e.index);
  }

  bool is_alive(Entity e) const {
    return e.index < versions.size() && versions[e.index] == e.version;
  }

  // -------------------------------------------
  // Component registration & ids
  // -------------------------------------------
  // Get or assign a compact component id for type T
  template <typename T> size_t component_id() {
    auto it = type_to_id.find(std::type_index(typeid(T)));
    if (it != type_to_id.end())
      return it->second;

    size_t id = component_count++;
    type_to_id.emplace(std::type_index(typeid(T)), id);

    // ensure all per-entity masks and existing groups expand to hold new id
    expand_masks_for_new_component();

    return id;
  }

  // -------------------------------------------
  // Basic component API (add/get/has/remove)
  // -------------------------------------------
  template <typename T, typename... Args> T &add(Entity e, Args &&...args) {
    assert(is_alive(e));
    auto *store = get_or_create_storage<T>();
    size_t cid = store->comp_id;
    // insert component into storage
    T &comp = store->set.insert(e.index, T{std::forward<Args>(args)...});
    // set the bit in entity mask
    set_entity_bit(e.index, cid);
    return comp;
  }

  template <typename T> bool has(Entity e) {
    if (!is_alive(e))
      return false;
    auto *store = get_storage<T>();
    if (!store)
      return false;
    return store->set.contains(e.index);
  }

  template <typename T> T &get(Entity e) {
    assert(is_alive(e));
    auto *store = get_or_create_storage<T>();
    return store->set.get(e.index);
  }

  template <typename T> void remove(Entity e) {
    if (!is_alive(e))
      return;
    auto *store = get_storage<T>();
    if (!store)
      return;
    store->set.erase(e.index);
    reset_entity_bit(e.index, store->comp_id);
  }

  // -------------------------------------------
  // Groups: precomputed masks for sets of components
  // -------------------------------------------
  struct Group {
    // required_mask sized to mask_blocks (kept in sync by ECS)
    std::vector<uint64_t> required_mask;
  };

  // Create a group for a variadic list of component types
  template <typename... Components> Group create_group() {
    Group g;
    g.required_mask.assign(mask_blocks, 0ull);
    (set_bit_in_mask(g.required_mask, component_id<Components>()), ...);
    return g;
  }

  // Test whether entity has all components from a group
  inline bool matches_group(Entity e, const Group &g) const {
    if (!is_alive(e))
      return false;
    if (g.required_mask.size() != mask_blocks) {
      // if group was created earlier, its mask might be smaller; treat missing
      // blocks as zeros -> expand necessary:
      // entity can't match bits outside group's mask anyway.
      // ensure sizes are compatible by checking available blocks only.
      size_t min_blocks = std::min(g.required_mask.size(), mask_blocks);
      const uint64_t *e_mask =
          &entity_masks[static_cast<size_t>(e.index) * mask_blocks];
      const uint64_t *g_mask = &g.required_mask[0];
      // check min_blocks
      for (size_t i = 0; i < min_blocks; ++i)
        if ((e_mask[i] & g_mask[i]) != g_mask[i])
          return false;
      // any nonzero required bits outside entity mask range -> no match
      for (size_t i = min_blocks; i < g.required_mask.size(); ++i)
        if (g.required_mask[i] != 0)
          return false;
      return true;
    } else {
      const uint64_t *e_mask =
          &entity_masks[static_cast<size_t>(e.index) * mask_blocks];
      return BitMaskHelper::test_mask_match(e_mask, g.required_mask.data(),
                                            mask_blocks);
    }
  }

  template <typename T1, typename... Ts, typename Func> void view(Func &&fn) {
    // Ensure T1 storage exists
    auto *s1 = get_storage<T1>();
    if (!s1)
      return;

    // Gather all storages
    std::array<IStorageBase *, 1 + sizeof...(Ts)> stores = {
        s1, get_storage<Ts>()...};

    // If any storage is missing → no matching entities
    for (auto *st : stores)
      if (!st)
        return;

    // Pick the smallest storage to iterate
    IStorageBase *best = stores[0];
    for (auto *st : stores)
      if (st->dense_size() < best->dense_size())
        best = st;

    // Precompute mask of required components
    Group req;
    req.required_mask.assign(mask_blocks, 0ull);
    set_bits_in_mask_from_types<0, T1, Ts...>(req.required_mask);

    // Iterate
    best->for_each_entity_idx([&](uint32_t ent_index, size_t dense_pos) {
      const uint64_t *em = mask_ptr(ent_index);

      if (!BitMaskHelper::test_mask_match(em, req.required_mask.data(),
                                          mask_blocks))
        return;

      // Pass entity + components to fn
      call_with_components<T1, Ts...>(ent_index, fn);
    });
  }

private:
  // -------------------------------------------
  // Low-level storage & bookkeeping
  // -------------------------------------------

  // Interface that lets us iterate dense storage and fetch components
  // generically.
  struct IStorageBase {
    virtual ~IStorageBase() = default;
    virtual void erase_entity(uint32_t idx) = 0;
    virtual size_t dense_size() const = 0;
    // iterate each (entityIndex, dense_pos) calling function
    virtual void
    for_each_entity_idx(std::function<void(uint32_t, size_t)> f) = 0;
  };

  // T-specific storage wrapper that implements IStorageBase
  template <typename T> struct Storage : IStorageBase {
    Storage(size_t cid) : comp_id(cid) {}
    size_t comp_id;
    SparseSet<T> set;

    void erase_entity(uint32_t idx) override { set.erase(idx); }
    size_t dense_size() const override { return set.entities().size(); }

    void for_each_entity_idx(std::function<void(uint32_t, size_t)> f) override {
      const auto &ents = set.entities();
      for (size_t i = 0; i < ents.size(); ++i)
        f(ents[i], i);
    }

    // helper to get component reference if present
    T *get_if_present(uint32_t ent_idx) {
      if (!set.contains(ent_idx))
        return nullptr;
      return &set.get(ent_idx);
    }
  };

  // mapping type_index -> Storage<T> pointer (polymorphic via IStorageBase)
  std::unordered_map<std::type_index, std::unique_ptr<IStorageBase>>
      component_storages;

  // map type_index -> component_id
  std::unordered_map<std::type_index, size_t> type_to_id;
  size_t component_count; // number of registered component types

  // per-entity versioning & free list
  std::vector<uint32_t> versions;
  std::vector<uint32_t> free_list;

  // flat storage for entity masks: entity_masks[entity * mask_blocks +
  // block_index]
  std::vector<uint64_t> entity_masks;
  size_t mask_blocks;

  // -------------------------------------------
  // Helpers: storage getters, mask ops, resizing
  // -------------------------------------------
  template <typename T> Storage<T> *get_storage() {
    auto it = component_storages.find(std::type_index(typeid(T)));
    if (it == component_storages.end())
      return nullptr;
    return static_cast<Storage<T> *>(it->second.get());
  }

  template <typename T> bool storage_exists() const {
    return type_to_id.find(std::type_index(typeid(T))) != type_to_id.end();
  }

  template <typename T> Storage<T> *get_or_create_storage() {
    auto it = component_storages.find(std::type_index(typeid(T)));
    if (it != component_storages.end())
      return static_cast<Storage<T> *>(it->second.get());

    size_t cid = component_id<T>(); // registers a new id if necessary
    auto store = std::make_unique<Storage<T>>(cid);
    Storage<T> *ptr = store.get();
    component_storages.emplace(std::type_index(typeid(T)), std::move(store));
    return ptr;
  }

  // called whenever component_count increases to expand masks & groups
  void expand_masks_for_new_component() {
    size_t new_blocks = BitMaskHelper::blocks_for_bits(component_count);
    if (new_blocks == mask_blocks)
      return;

    // create new masks vector sized versions.size() * new_blocks
    std::vector<uint64_t> new_masks;
    new_masks.assign(static_cast<size_t>(versions.size()) * new_blocks, 0ull);

    // copy old masks into new layout
    for (size_t ent = 0; ent < versions.size(); ++ent) {
      const uint64_t *old_ptr = mask_ptr(ent);
      uint64_t *new_ptr = &new_masks[ent * new_blocks];
      if (mask_blocks > 0) {
        for (size_t b = 0; b < mask_blocks; ++b)
          new_ptr[b] = old_ptr[b];
        for (size_t b = mask_blocks; b < new_blocks; ++b)
          new_ptr[b] = 0;
      }
    }

    entity_masks.swap(new_masks);
    mask_blocks = new_blocks;
  }

  // ensure entity_masks is large enough for 'entities' entities
  void ensure_entity_mask_size(size_t entities) {
    if (mask_blocks == 0) {
      // still zero: make sure entity_masks has zero length entries
      entity_masks.resize(entities * mask_blocks);
      return;
    }
    if (entity_masks.size() < entities * mask_blocks) {
      entity_masks.resize(entities * mask_blocks, 0ull);
    }
  }

  // returns pointer to entity's mask first block (may return pointer into empty
  // vector)
  inline uint64_t *mask_ptr_mut(size_t ent_index) {
    if (mask_blocks == 0)
      return nullptr;
    return &entity_masks[ent_index * mask_blocks];
  }

  inline const uint64_t *mask_ptr(size_t ent_index) const {
    if (mask_blocks == 0)
      return nullptr;
    return &entity_masks[ent_index * mask_blocks];
  }

  inline void set_entity_bit(size_t ent_index, size_t comp_id) {
    // ensure masks large enough
    if (comp_id >= component_count)
      return; // should not happen
    size_t needed_blocks = BitMaskHelper::blocks_for_bits(component_count);
    if (needed_blocks != mask_blocks)
      expand_masks_for_new_component();
    ensure_entity_mask_size(versions.size());
    uint64_t *m = mask_ptr_mut(ent_index);
    BitMaskHelper::set_bit(m, comp_id);
  }

  inline void reset_entity_bit(size_t ent_index, size_t comp_id) {
    if (mask_blocks == 0)
      return;
    uint64_t *m = mask_ptr_mut(ent_index);
    BitMaskHelper::reset_bit(m, comp_id);
  }

  inline void set_bit_in_mask(std::vector<uint64_t> &mask_vec, size_t bit) {
    size_t bidx = bit / BitMaskHelper::BLOCK_BITS;
    if (bidx >= mask_vec.size())
      mask_vec.resize(bidx + 1, 0ull);
    mask_vec[bidx] |= (uint64_t(1) << (bit % BitMaskHelper::BLOCK_BITS));
  }

  // helper to set bits for variadic types in mask vector (used by view for
  // precomputed req mask)
  template <size_t I = 0, typename TFirst, typename... TRest>
  static void
  set_bits_in_mask_from_types_impl(std::vector<uint64_t> &mask_vec) {
    (void)mask_vec; // placeholder
  }

  template <size_t I = 0, typename TFirst, typename... TRest>
  void set_bits_in_mask_from_types(std::vector<uint64_t> &mask_vec) {
    // expand mask_vec to current mask_blocks
    if (mask_vec.size() < mask_blocks)
      mask_vec.assign(mask_blocks, 0ull);
    // set TFirst
    set_bit_in_mask(mask_vec, component_id<TFirst>());
    // set remaining recursively using fold-expression style
    (set_bit_in_mask(mask_vec, component_id<TRest>()), ...);
  }

  // wrapper to call user function with components references
  template <typename T1, typename... Ts, typename Func>
  void call_with_components(uint32_t ent_index, Func &fn) {
    T1 &c1 = get_storage<T1>()->set.get(ent_index);
    // for others, we need to fetch each component reference
    auto tup =
        std::forward_as_tuple(c1, (get_storage<Ts>()->set.get(ent_index))...);
    std::apply(
        [&](auto &...args) {
          fn(Entity{ent_index, versions[ent_index]}, args...);
        },
        tup);
  }
};
