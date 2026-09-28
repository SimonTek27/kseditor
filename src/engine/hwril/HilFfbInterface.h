#pragma once
#include <string>
namespace ks { namespace engine { namespace hwril {
class HilFfbInterface {
public:
    static HilFfbInterface& instance() { static HilFfbInterface s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void setTorque(float /*nm*/) {}
};
}}} // namespace
