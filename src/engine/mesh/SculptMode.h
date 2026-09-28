#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class SculptMode {
public:
    static SculptMode& instance() { static SculptMode s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
