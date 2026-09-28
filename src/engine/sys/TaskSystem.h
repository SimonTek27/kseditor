#pragma once
#include <functional>
#include <string>
namespace ks { namespace engine { namespace sys {
class TaskSystem {
public:
    static TaskSystem& instance() { static TaskSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void enqueue(std::function<void()> fn) { if (fn) fn(); }
};
}}} // namespace
