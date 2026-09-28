#pragma once
#include <string>
#include "MeshData.h"
namespace ks { namespace engine { namespace fileformat {
class KS3DReader {
public:
    bool open(const std::string& /*path*/) { return false; }
    MeshData mesh() const { return {}; }
};
}}} // namespace
