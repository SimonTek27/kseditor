#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace ks {

// Connection handle returned by Signal::connect(). Like Qt's
// QMetaObject::Connection it does NOT disconnect on destruction: keep the id
// and call disconnect() when the listener goes away.
using ConnectionId = std::uint64_t;
constexpr ConnectionId INVALID_CONNECTION = 0;

// Minimal signal/slot replacement for Qt's signals/slots. Slots are invoked in
// connect order. A slot may connect or disconnect (including itself) during
// emission: erasure is deferred until the outermost emission finishes, so no
// allocation happens on the emit path.
template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    ~Signal() { disconnectAll(); }

    ConnectionId connect(Slot slot) {
        const ConnectionId id = ++m_nextId;
        m_entries.push_back(Entry{id, std::move(slot), false});
        return id;
    }

    template <typename T>
    ConnectionId connect(T* obj, void (T::*method)(Args...)) {
        return connect([obj, method](Args... args) { (obj->*method)(args...); });
    }

    void disconnect(ConnectionId id) {
        if (id == INVALID_CONNECTION) return;
        for (auto& e : m_entries) {
            if (e.id != id) continue;
            e.slot = nullptr;
            e.dead = true;
            break;
        }
        if (m_depth == 0) compact();
    }

    void disconnectAll() {
        for (auto& e : m_entries) {
            e.slot = nullptr;
            e.dead = true;
        }
        if (m_depth == 0) compact();
    }

    std::size_t connectionCount() const {
        std::size_t n = 0;
        for (const auto& e : m_entries) n += (e.slot ? 1u : 0u);
        return n;
    }

    void operator()(Args... args) {
        ++m_depth;
        const std::size_t n = m_entries.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto& e = m_entries[i];
            if (e.dead || !e.slot) continue;
            e.slot(args...);
        }
        if (--m_depth == 0) compact();
    }

private:
    struct Entry {
        ConnectionId id;
        Slot slot;
        bool dead;
    };

    void compact() {
        m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                       [](const Entry& e) { return e.dead; }),
                        m_entries.end());
    }

    std::vector<Entry> m_entries;
    ConnectionId m_nextId = 0;
    int m_depth = 0;
};

// RAII wrapper: disconnects the bound signal when destroyed. Move-only.
template <typename... Args>
class ScopedConnection {
public:
    ScopedConnection() = default;
    ScopedConnection(Signal<Args...>& sig, ConnectionId id) : m_sig(&sig), m_id(id) {}
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&& o) noexcept : m_sig(o.m_sig), m_id(o.m_id) {
        o.m_sig = nullptr;
        o.m_id = INVALID_CONNECTION;
    }
    ScopedConnection& operator=(ScopedConnection&& o) noexcept {
        if (this != &o) {
            disconnect();
            m_sig = o.m_sig;
            m_id = o.m_id;
            o.m_sig = nullptr;
            o.m_id = INVALID_CONNECTION;
        }
        return *this;
    }
    ~ScopedConnection() { disconnect(); }

    void disconnect() {
        if (m_sig) m_sig->disconnect(m_id);
        m_sig = nullptr;
        m_id = INVALID_CONNECTION;
    }

    bool isConnected() const { return m_sig != nullptr; }

private:
    Signal<Args...>* m_sig = nullptr;
    ConnectionId m_id = INVALID_CONNECTION;
};

}  // namespace ks
