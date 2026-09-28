#pragma once
#include <string>
namespace ks { namespace engine { namespace network {
class NetSystem {
public:
    static NetSystem& instance() { static NetSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
