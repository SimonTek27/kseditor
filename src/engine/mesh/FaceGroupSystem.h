#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class FaceGroupSystem {
public:
    static FaceGroupSystem& instance() { static FaceGroupSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
