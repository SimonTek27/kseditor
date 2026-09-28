#pragma once
#include <string>
namespace ks { namespace engine { namespace assets {
class Paths {
public:
    static std::string projectRoot() { return "."; }
    static std::string content() { return "content"; }
    static std::string shaders() { return "shaders"; }
};
}}} // namespace
