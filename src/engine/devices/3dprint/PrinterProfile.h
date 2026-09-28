#pragma once
#include <string>
namespace ks { namespace device { namespace print3d {
struct PrinterProfile {
    std::string name;
    float nozzleMm = 0.4f;
};
}}} // namespace
