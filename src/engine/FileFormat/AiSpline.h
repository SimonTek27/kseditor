#pragma once
#include <string>
#include <vector>
namespace ks { namespace engine { namespace fileformat {
struct AiSplinePoint { float x=0,y=0,z=0; float speed=0; };
class AiSpline {
public:
    bool load(const std::string& /*path*/) { return false; }
    const std::vector<AiSplinePoint>& points() const { return m_pts; }
private:
    std::vector<AiSplinePoint> m_pts;
};
}}} // namespace
