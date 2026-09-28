#pragma once
#include <string>

namespace ks {
namespace adapters {
namespace assetto_corsa {

class FSPROExporter {
public:
    bool exportFile(const std::string& /*path*/) { return false; }
};

} // namespace assetto_corsa
} // namespace adapters
} // namespace ks
