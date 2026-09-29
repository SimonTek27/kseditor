#pragma once
// ks::ecs::Registry — sparse-set entity/component store, Qt-free and
// dependency-free (no entt/boost: ksengine has to stay a self-contained lib).
// Handles are (generation << 20 | index) so a stale handle can never alias a
// recycled slot. One pool per component type, O(1) emplace/get/erase,
// O(smallest pool) iteration for multi-component views.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ks::ecs {

using Entity = uint32_t;

inline constexpr Entity kNullEntity = 0xFFFFFFFFu;
inline constexpr uint32_t kIndexBits = 20;
inline constexpr uint32_t kGenerationBits = 12;
inline constexpr uint32_t kIndexMask = (1u << kIndexBits) - 1u;
inline constexpr uint32_t kGenerationMask = (1u << kGenerationBits) - 1u;
inline constexpr uint32_t kMaxEntities = kIndexMask;

inline constexpr uint32_t entityIndex(Entity e) noexcept { return e & kIndexMask; }
inline constexpr uint32_t entityGeneration(Entity e) noexcept { return e >> kIndexBits; }
inline constexpr Entity makeEntity(uint32_t index, uint32_t generation) noexcept {
    return ((generation & kGenerationMask) << kIndexBits) | (index & kIndexMask);
}

class PoolBase {
public:
    virtual ~PoolBase() = default;
    virtual bool has(Entity e) const = 0;
    virtual void erase(Entity e) = 0;
    virtual std::size_t size() const = 0;
    virtual const std::vector<Entity>& entities() const = 0;
};

template <typename T>
class Pool final : public PoolBase {
public:
    bool has(Entity e) const override {
        const uint32_t idx = entityIndex(e);
        if (idx >= m_sparse.size()) return false;
        const uint32_t dense = m_sparse[idx];
        return dense != kInvalidDense && dense < m_entities.size() && m_entities[dense] == e;
    }

    template <typename... A>
    T& emplace(Entity e, A&&... args) {
        const uint32_t idx = entityIndex(e);
        if (idx >= m_sparse.size()) m_sparse.resize(idx + 1u, kInvalidDense);
        const uint32_t dense = m_sparse[idx];
        if (dense != kInvalidDense && dense < m_entities.size() && m_entities[dense] == e) {
            m_dense[dense] = T(std::forward<A>(args)...);
            return m_dense[dense];
        }
        m_dense.emplace_back(std::forward<A>(args)...);
        m_entities.push_back(e);
        m_sparse[idx] = static_cast<uint32_t>(m_dense.size() - 1u);
        return m_dense.back();
    }

    void erase(Entity e) override {
        if (!has(e)) return;
        const uint32_t idx = entityIndex(e);
        const uint32_t dense = m_sparse[idx];
        const uint32_t last = static_cast<uint32_t>(m_dense.size() - 1u);
        if (dense != last) {
            m_dense[dense] = std::move(m_dense[last]);
            const Entity moved = m_entities[last];
            m_entities[dense] = moved;
            m_sparse[entityIndex(moved)] = dense;
        }
        m_dense.pop_back();
        m_entities.pop_back();
        m_sparse[idx] = kInvalidDense;
    }

    T& get(Entity e) {
        assert(has(e));
        return m_dense[m_sparse[entityIndex(e)]];
    }

    const T& get(Entity e) const {
        assert(has(e));
        return m_dense[m_sparse[entityIndex(e)]];
    }

    T* tryGet(Entity e) { return has(e) ? &m_dense[m_sparse[entityIndex(e)]] : nullptr; }

    std::size_t size() const override { return m_dense.size(); }
    const std::vector<Entity>& entities() const override { return m_entities; }

private:
    static constexpr uint32_t kInvalidDense = 0xFFFFFFFFu;
    std::vector<uint32_t> m_sparse;
    std::vector<T> m_dense;
    std::vector<Entity> m_entities;
};

class Registry {
public:
    Entity create() {
        uint32_t idx;
        if (!m_free.empty()) {
            idx = m_free.back();
            m_free.pop_back();
        } else {
            idx = static_cast<uint32_t>(m_generations.size());
            if (idx >= kMaxEntities) return kNullEntity;
            m_generations.push_back(0);
        }
        ++m_alive;
        return makeEntity(idx, m_generations[idx]);
    }

    void destroy(Entity e) {
        if (!valid(e)) return;
        const uint32_t idx = entityIndex(e);
        for (auto& kv : m_pools) kv.second->erase(e);
        m_generations[idx] =
            static_cast<uint16_t>((m_generations[idx] + 1u) & kGenerationMask);
        m_free.push_back(idx);
        --m_alive;
    }

    bool valid(Entity e) const {
        if (e == kNullEntity) return false;
        const uint32_t idx = entityIndex(e);
        return idx < m_generations.size() && m_generations[idx] == entityGeneration(e);
    }

    std::size_t alive() const { return m_alive; }

    void clear() {
        m_pools.clear();
        m_generations.clear();
        m_free.clear();
        m_alive = 0;
    }

    template <typename T, typename... A>
    T& emplace(Entity e, A&&... args) {
        assert(valid(e) && "emplace on a dead entity");
        return pool<T>().emplace(e, std::forward<A>(args)...);
    }

    template <typename T>
    void remove(Entity e) {
        if (valid(e)) pool<T>().erase(e);
    }

    template <typename T>
    bool has(Entity e) const {
        if (!valid(e)) return false;
        const Pool<T>* p = poolIf<T>();
        return p && p->has(e);
    }

    template <typename T>
    T* tryGet(Entity e) {
        if (!valid(e)) return nullptr;
        return pool<T>().tryGet(e);
    }

    template <typename T>
    T& get(Entity e) {
        assert(valid(e) && "get on a dead entity");
        return pool<T>().get(e);
    }

    template <typename... T, typename F>
    void each(F&& fn) {
        static_assert(sizeof...(T) >= 1, "each<> requires at least one component type");
        static_assert(sizeof...(T) <= 4, "each<> supports up to four component types");
        const PoolBase* driver = nullptr;
        const auto consider = [&](const PoolBase* p) {
            if (!driver || p->size() < driver->size()) driver = p;
        };
        (consider(&pool<T>()), ...);
        if (!driver || driver->size() == 0) return;

        const std::vector<Entity> ids = driver->entities();
        for (Entity e : ids) {
            if (!valid(e)) continue;
            bool matches = true;
            ((matches = matches && pool<T>().has(e)), ...);
            if (matches) fn(e, pool<T>().get(e)...);
        }
    }

    template <typename... T, typename F>
    void each(F&& fn) const {
        static_assert(sizeof...(T) >= 1, "each<> requires at least one component type");
        static_assert(sizeof...(T) <= 4, "each<> supports up to four component types");
        const PoolBase* driver = nullptr;
        bool missing = false;
        const auto consider = [&](const PoolBase* p) {
            if (!p) {
                missing = true;
                return;
            }
            if (!driver || p->size() < driver->size()) driver = p;
        };
        (consider(poolIf<T>()), ...);
        if (missing || !driver || driver->size() == 0) return;

        const std::vector<Entity> ids = driver->entities();
        for (Entity e : ids) {
            if (!valid(e)) continue;
            bool matches = true;
            ((matches = matches && poolIf<T>()->has(e)), ...);
            if (matches) fn(e, poolIf<T>()->get(e)...);
        }
    }

private:
    template <typename T>
    Pool<T>& pool() {
        const std::type_index key(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) it = m_pools.emplace(key, std::make_unique<Pool<T>>()).first;
        return *static_cast<Pool<T>*>(it->second.get());
    }

    template <typename T>
    const Pool<T>* poolIf() const {
        const auto it = m_pools.find(std::type_index(typeid(T)));
        if (it == m_pools.end()) return nullptr;
        return static_cast<const Pool<T>*>(it->second.get());
    }

    std::unordered_map<std::type_index, std::unique_ptr<PoolBase>> m_pools;
    std::vector<uint16_t> m_generations;
    std::vector<uint32_t> m_free;
    std::size_t m_alive = 0;
};

} // namespace ks::ecs
