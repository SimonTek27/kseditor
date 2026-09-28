#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class KSAudioValidator {
public:
    bool validate(const std::string& /*path*/) { return true; }
};
}}} // namespace
