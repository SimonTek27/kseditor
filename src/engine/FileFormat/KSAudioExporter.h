#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class KSAudioExporter {
public:
    bool exportFile(const std::string& /*path*/) { return false; }
};
}}} // namespace
