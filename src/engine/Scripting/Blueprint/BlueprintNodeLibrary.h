#pragma once
#include <string>
namespace ks { namespace scripting {
class BlueprintNodeLibrary {
public:
    static BlueprintNodeLibrary& instance() { static BlueprintNodeLibrary s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}} // namespace
