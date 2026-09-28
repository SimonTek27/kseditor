#pragma once
/** 3D-print tooling is editor-only — not part of sim runtime. */
#include <string>
namespace ks { namespace device { namespace print3d {
struct PrintJob { std::string name; };
}}} // namespace
