#pragma once
#include <string>
#include <functional>
#include <vector>

namespace ks {
namespace engine {
namespace sys {

class TransactionManager {
public:
    static TransactionManager& instance() { static TransactionManager s; return s; }

    void begin(const std::string& name = {}) {
        m_active = true;
        m_name = name;
    }
    void commit() {
        m_active = false;
        for (auto& fn : m_onCommit) if (fn) fn();
        m_onCommit.clear();
    }
    void rollback() {
        m_active = false;
        for (auto& fn : m_onRollback) if (fn) fn();
        m_onRollback.clear();
        m_onCommit.clear();
    }
    bool isActive() const { return m_active; }

    void onCommit(std::function<void()> fn) { m_onCommit.push_back(std::move(fn)); }
    void onRollback(std::function<void()> fn) { m_onRollback.push_back(std::move(fn)); }

private:
    bool m_active = false;
    std::string m_name;
    std::vector<std::function<void()>> m_onCommit;
    std::vector<std::function<void()>> m_onRollback;
};

} // namespace sys
} // namespace engine
} // namespace ks
