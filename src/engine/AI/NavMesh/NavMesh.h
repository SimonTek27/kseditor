#pragma once
#include <string>
#include <vector>
namespace ks { namespace ai {
struct NavPoint { float x=0,y=0,z=0; };
class NavMesh {
public:
    static NavMesh& instance() { static NavMesh s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool load(const std::string& /*path*/) { return false; }
    std::vector<NavPoint> findPath(NavPoint, NavPoint) const { return {}; }
};
}} // namespace
