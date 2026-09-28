#pragma once
#include <string>
namespace ks { namespace engine { namespace animation {
class ShapeKeyAnimDriver {
public:
    static ShapeKeyAnimDriver& instance() { static ShapeKeyAnimDriver s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
