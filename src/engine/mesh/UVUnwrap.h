#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class UVUnwrap {
public:
    static UVUnwrap& instance() { static UVUnwrap s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
