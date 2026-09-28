#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class FSPROImporter {
public:
    bool importFile(const std::string& /*path*/) { return false; }
};
}}} // namespace
