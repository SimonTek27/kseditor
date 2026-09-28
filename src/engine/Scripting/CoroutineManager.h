#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace ks {
namespace scripting {

class CoroutineManager {
public:
    using Id = uint64_t;
    static CoroutineManager& instance() { static CoroutineManager s; return s; }
    Id start(const std::string& /*name*/, std::function<void()> fn) {
        Id id = ++m_next;
        if (fn) fn();
        return id;
    }
    void stop(Id /*id*/) {}
    void update(float /*dt*/) {}
private:
    Id m_next = 0;
};

} // namespace scripting
} // namespace ks
