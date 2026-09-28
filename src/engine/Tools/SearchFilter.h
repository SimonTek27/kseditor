#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class SearchFilter {
public:
    static SearchFilter& instance() { static SearchFilter s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
