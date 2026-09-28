#pragma once
#include <string>

namespace ks {
namespace engine {
namespace graphics {

class ComputePipeline {
public:
    static ComputePipeline& instance() { static ComputePipeline s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool dispatch(const std::string& /*shader*/, int /*gx*/, int /*gy*/, int /*gz*/) { return false; }
};

} // namespace graphics
} // namespace engine
} // namespace ks
