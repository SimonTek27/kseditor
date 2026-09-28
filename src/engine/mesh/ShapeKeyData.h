#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class ShapeKeyData {
public:
    static ShapeKeyData& instance() { static ShapeKeyData s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
