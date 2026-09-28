#pragma once
/** Qt-free Streamline stubs (optional NVIDIA DLSS path). */
#include <cstdint>
#include <cstdio>

namespace ks {
namespace streamline {

inline bool isAvailable() { return false; }
inline bool initialize() {
    std::fprintf(stderr, "Streamline: not linked (optional)\n");
    return false;
}
inline void shutdown() {}

} // namespace streamline
} // namespace ks
