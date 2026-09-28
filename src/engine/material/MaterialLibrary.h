#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class MaterialLibrary {
public:
    static MaterialLibrary& instance() { static MaterialLibrary s; return s; }
    bool load(const std::string&) { return false; }
};
}}} // namespace
