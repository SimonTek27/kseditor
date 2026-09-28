#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class MultiresLevel {
public:
    static MultiresLevel& instance() { static MultiresLevel s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
