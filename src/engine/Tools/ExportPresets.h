#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class ExportPresets {
public:
    static ExportPresets& instance() { static ExportPresets s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
