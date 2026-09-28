#pragma once
/** Qt-free compute pipeline facade. */
#include <string>
#include <vector>
#include <cstdint>

namespace ks {

class VulkanComputePipeline {
public:
    VulkanComputePipeline() = default;
    ~VulkanComputePipeline() = default;

    bool initialize(const std::string& /*computeShaderPath*/) { return false; }
    void destroy() {}

    void dispatch(int /*workGroupX*/, int /*workGroupY*/, int /*workGroupZ*/) {}
    void setUniform(const std::string& /*name*/, float /*value*/) {}
    std::vector<float> readBackResults() const { return {}; }
};

} // namespace ks
