#pragma once
// ks::Engine module that owns the ordered list of ECS systems. Engine::tick()
// calls update() at the fixed timestep, which runs every registered system
// against Engine::registry() — the ksengine equivalent of a per-frame system
// scheduler.
#include "../EngineModule.h"
#include "Registry.h"
#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ks::ecs {

class SceneModule : public ::ks::EngineModule {
public:
    using System = std::function<void(Registry&, double)>;

    static SceneModule& instance() {
        static SceneModule s;
        return s;
    }

    std::string moduleName() const override { return "SceneModule"; }
    std::string moduleId() const override { return "ks.scene"; }

    void attach(Registry* reg) { m_registry = reg; }
    Registry* registry() const { return m_registry; }

    bool addSystem(const std::string& name, System sys) {
        if (!sys) return false;
        removeSystem(name);
        m_systems.push_back({name, std::move(sys)});
        return true;
    }

    void removeSystem(const std::string& name) {
        m_systems.erase(std::remove_if(m_systems.begin(), m_systems.end(),
                                       [&](const Entry& e) { return e.name == name; }),
                        m_systems.end());
    }

    void clearSystems() { m_systems.clear(); }

    int systemCount() const { return static_cast<int>(m_systems.size()); }

    bool hasSystem(const std::string& name) const {
        for (const Entry& e : m_systems)
            if (e.name == name) return true;
        return false;
    }

    void update(double dt) override {
        if (!m_registry) return;
        for (Entry& e : m_systems)
            if (e.system) e.system(*m_registry, dt);
    }

private:
    struct Entry {
        std::string name;
        System system;
    };

    std::vector<Entry> m_systems;
    Registry* m_registry = nullptr;
};

} // namespace ks::ecs
