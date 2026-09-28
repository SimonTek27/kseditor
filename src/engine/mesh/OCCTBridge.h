#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class OCCTBridge {
public:
    static OCCTBridge& instance() { static OCCTBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
