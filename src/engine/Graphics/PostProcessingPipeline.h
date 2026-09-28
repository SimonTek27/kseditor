#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class PostProcessingPipeline {
public:
    static PostProcessingPipeline& instance() { static PostProcessingPipeline s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "PostProcessingPipeline"; }
};
}}} // namespace
