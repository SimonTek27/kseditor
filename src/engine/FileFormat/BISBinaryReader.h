#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class BISBinaryReader {
public:
    bool open(const std::string& /*path*/) { return false; }
};
}}} // namespace
