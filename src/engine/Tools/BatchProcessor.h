#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class BatchProcessor {
public:
    static BatchProcessor& instance() { static BatchProcessor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
