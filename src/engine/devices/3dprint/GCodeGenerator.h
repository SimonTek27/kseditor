#pragma once
#include <string>
namespace ks { namespace device { namespace print3d {
class GCodeGenerator {
public:
    bool generate(const std::string& /*outPath*/) { return false; }
};
}}} // namespace
