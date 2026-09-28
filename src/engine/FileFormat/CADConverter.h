#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class CADConverter {
public:
    static bool convert(const std::string& /*inPath*/, const std::string& /*outPath*/) { return false; }
};
}}} // namespace
