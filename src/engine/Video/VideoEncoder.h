#pragma once
#include <string>
namespace ks { namespace engine { namespace video {
class VideoEncoder {
public:
    static VideoEncoder& instance() { static VideoEncoder s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool start(const std::string& /*path*/) { return false; }
    void stop() {}
};
}}} // namespace
