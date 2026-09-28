#pragma once
#include <string>
#include "MeshData.h"
namespace ks { namespace engine { namespace fileformat {
class KS3DWriter {
public:
    bool write(const std::string& /*path*/, const MeshData&) { return false; }
};
}}} // namespace
