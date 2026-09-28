#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class ParamFile {
public:
    bool load(const std::string& /*path*/) { return false; }
    std::string get(const std::string& /*key*/, const std::string& def = "") const { return def; }
};
}}} // namespace
