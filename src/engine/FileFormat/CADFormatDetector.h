#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
enum class CADFormat { Unknown, OBJ, STL, DXF, FBX, GLB };
class CADFormatDetector {
public:
    static CADFormat detect(const std::string& /*path*/) { return CADFormat::Unknown; }
};
}}} // namespace
