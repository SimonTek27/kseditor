#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class CADDXFParser {
public:
    bool load(const std::string& /*path*/) { return false; }
};
}}} // namespace
