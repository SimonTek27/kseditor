#pragma once
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QVector>
#include <QReadWriteLock>
#include <QDebug>
#include <QtGlobal>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <chrono>
#include <algorithm>
#include "EngineModule.h"
#include "sys/SystemDllInitializer.h"

namespace ks::engine {

using Entity = uint32_t;
constexpr Entity INVALID_ENTITY = 0;

class Registry {
public:
    Entity create() { return ++m_nextId; }
    void destroy(Entity e) {
        for (auto &kv : m_storages) kv.second->remove(e);
    }
    template<typename T, typename... Args>
    T& emplace(Entity e, Args&&... args) {
        auto &s = storage<T>();
        return s.emplace(e, T(std::forward<Args>(args)...));
    }
    template<typename T>
    T* get(Entity e) {
        auto &s = storage<T>();
        return s.get(e);
    }
    template<typename T>
    bool has(Entity e) const {
        auto it = m_storages.find(std::type_index(typeid(T)));
        if (it == m_storages.end()) return false;
        return static_cast<Storage<T>*>(it->second.get())->has(e);
    }
    template<typename T>
    void remove(Entity e) { storage<T>().remove(e); }

    template<typename T>
    QVector<Entity> view() const {
        auto it = m_storages.find(std::type_index(typeid(T)));
        if (it == m_storages.end()) return {};
        return static_cast<Storage<T>*>(it->second.get())->entities();
    }

private:
    struct IStorage { virtual ~IStorage(){} virtual void remove(Entity)=0; };
    template<typename T>
    struct Storage : IStorage {
        QHash<Entity,T> data;
        T& emplace(Entity e, T v){ data[e]=std::move(v); return data[e]; }
        T* get(Entity e){ auto it=data.find(e); return it==data.end()?nullptr:&it.value(); }
        bool has(Entity e) const { return data.contains(e); }
        void remove(Entity e) override { data.remove(e); }
        QVector<Entity> entities() const { return QVector<Entity>(data.keys().begin(), data.keys().end()); }
    };
    template<typename T>
    Storage<T>& storage() {
        auto idx = std::type_index(typeid(T));
        auto it = m_storages.find(idx);
        if (it==m_storages.end()) {
            auto s = std::make_unique<Storage<T>>();
            auto *ptr = s.get();
            m_storages[idx]=std::move(s);
            return *ptr;
        }
        return *static_cast<Storage<T>*>(it->second.get());
    }
    Entity m_nextId = 0;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> m_storages;
};

class EngineLoop : public QObject {
    Q_OBJECT
public:
    explicit EngineLoop(QObject* parent=nullptr) : QObject(parent) {
        m_timer.setTimerType(Qt::PreciseTimer);
        connect(&m_timer, &QTimer::timeout, this, &EngineLoop::onTick);
    }
    void start(double fixedDt = 0.001) {
        m_fixedDt = fixedDt;
        m_accum = 0;
        m_clock = std::chrono::steady_clock::now();
        m_timer.start(qMax(1, int(fixedDt*1000.0)));
        emit started();
    }
    void stop(){ m_timer.stop(); emit stopped(); }
    bool isRunning() const { return m_timer.isActive(); }
    void setFixedDt(double dt){ m_fixedDt = dt; }
    double fixedDt() const { return m_fixedDt; }
signals:
    void tick(double dt);
    void started();
    void stopped();
private slots:
    void onTick(){
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - m_clock).count();
        m_clock = now;
        elapsed = qBound(0.0, elapsed, 0.1);
        m_accum += elapsed;
        while (m_accum >= m_fixedDt){
            emit tick(m_fixedDt);
            m_accum -= m_fixedDt;
        }
    }
private:
    QTimer m_timer;
    std::chrono::steady_clock::time_point m_clock;
    double m_fixedDt = 0.001;
    double m_accum = 0;
};

class Engine : public QObject {
    Q_OBJECT
public:
    static Engine& instance(){
        static Engine e;
        return e;
    }
    bool initialize(){
        if (m_initialized) return true;

        // Initialize system DLLs before modules
        auto* dllInit = sys::SystemDllInitializer::instance();
        auto initResult = dllInit->initializeAll();
        if (initResult.totalFailed > 0) {
            qWarning() << "Engine: Some system DLLs failed to load:"
                       << initResult.totalFailed << "failed";
            for (const QString& error : initResult.errors) {
                qWarning() << "  -" << error;
            }
        }

        std::sort(m_modules.begin(), m_modules.end(), [](auto *a, auto *b){ return a->priority() > b->priority(); });
        for (auto *m : m_modules) if (!m->initialize()) { qWarning() << "Engine: module failed" << m->moduleName(); return false; }
        connect(&m_loop, &EngineLoop::tick, this, &Engine::onFixedTick);
        m_initialized = true;
        emit initialized();
        return true;
    }
    void shutdown(){
        if (!m_initialized) return;
        m_loop.stop();
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) (*it)->shutdown();

        // Shutdown system DLLs after modules
        auto* dllInit = sys::SystemDllInitializer::instance();
        dllInit->shutdownAll();

        m_initialized = false;
        emit shutdownCompleted();
    }
    void start(){ m_loop.start(m_fixedDt); emit started(); }
    void stop(){ m_loop.stop(); emit stopped(); }
    bool isRunning() const { return m_loop.isRunning(); }

    void registerModule(EngineModule* m){ m_modules.append(m); }
    void unregisterModule(EngineModule* m){ m_modules.removeAll(m); }

    Registry& registry(){ return m_registry; }
    EngineLoop& loop(){ return m_loop; }
    double fixedDt() const { return m_fixedDt; }
    void setFixedDt(double dt){ m_fixedDt = dt; m_loop.setFixedDt(dt); }
    uint64_t tickCount() const { return m_tickCount; }
    double time() const { return m_tickCount * m_fixedDt; }

    template<typename T>
    void addSystem(std::function<void(double)> fn){ m_systems.append(fn); }

signals:
    void initialized();
    void shutdownCompleted();
    void started();
    void stopped();
    void fixedTick(double dt);

private slots:
    void onFixedTick(double dt){
        ++m_tickCount;
        for (auto &fn : m_systems) fn(dt);
        emit fixedTick(dt);
    }

private:
    Engine() = default;
    bool m_initialized = false;
    double m_fixedDt = 0.001;
    uint64_t m_tickCount = 0;
    EngineLoop m_loop;
    Registry m_registry;
    QVector<EngineModule*> m_modules;
    QVector<std::function<void(double)>> m_systems;
};

} // namespace ks::engine
