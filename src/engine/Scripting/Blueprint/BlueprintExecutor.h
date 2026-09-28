#pragma once
#include <string>
namespace ks { namespace scripting {
class BlueprintExecutor {
public:
    static BlueprintExecutor& instance() { static BlueprintExecutor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}} // namespace
