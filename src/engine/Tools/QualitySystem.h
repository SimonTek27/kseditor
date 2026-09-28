#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class QualitySystem {
public:
    static QualitySystem& instance() { static QualitySystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
