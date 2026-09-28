#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class SSRSystem {
public:
    static SSRSystem& instance() { static SSRSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
