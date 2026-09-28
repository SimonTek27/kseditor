#pragma once
#include <string>
#include <vector>
namespace ks { namespace engine { namespace fileformat {
struct CADVertex { float x=0,y=0,z=0; };
struct CADMesh { std::string name; std::vector<CADVertex> verts; std::vector<uint32_t> indices; };
}}} // namespace
