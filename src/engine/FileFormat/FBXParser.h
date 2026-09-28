#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class FBXParser {
public:
    bool load(const std::string& /*path*/) { return false; }
};
}}} // namespace
