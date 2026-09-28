#pragma once
#include <string>
#include "CADTypes.h"
namespace ks { namespace engine { namespace fileformat {
class CADOBJParser {
public:
    bool load(const std::string& /*path*/) { return false; }
    const CADMesh& mesh() const { return m_mesh; }
private:
    CADMesh m_mesh;
};
}}} // namespace
