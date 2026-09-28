#pragma once
#include <string>
namespace ks { namespace engine { namespace network {
class NetRace {
public:
    static NetRace& instance() { static NetRace s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
