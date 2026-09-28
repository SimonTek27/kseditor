#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class BooleanOps {
public:
    static BooleanOps& instance() { static BooleanOps s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
