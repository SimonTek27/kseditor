#pragma once
/** AC / FMOD bank import — content adapter. */
#include <string>

namespace ks {
namespace adapters {
namespace assetto_corsa {

class FSPROImporter {
public:
    bool importFile(const std::string& /*path*/) { return false; }
};

} // namespace assetto_corsa
} // namespace adapters
} // namespace ks
