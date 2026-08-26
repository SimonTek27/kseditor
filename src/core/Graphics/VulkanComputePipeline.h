#pragma once

#include "VulkanRenderer.h"
#include "VulkanShaderLoader.h"
#include <QString>
#include <QVector3D>
#include <QMatrix4x4>

namespace ks {

class VulkanComputePipeline : public QObject {
    Q_OBJECT
public:
    VulkanComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue computeQueue);
    ~VulkanComputePipeline();

    bool initialize(const QString& computeShaderPath);
    void destroy();

    // Dispatch compute shader with given work group size
    void dispatch(int workGroupX, int workGroupY, int workGroupZ);

    // Update uniform data
    void setUniform(const QString& name, const QVariant& value);

    // Read back results (for debugging/visualization)
    QVector<float> readBackResults(int width, int height);

    // Get the pipeline layout
    VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }

    // Get the compute pipeline
    VkPipeline computePipeline() const { return m_computePipeline; }

signals:
    void initialized();
    void error(const QString& message);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    // Storage buffers for simulation data
    std::unique_ptr<VulkanBuffer> m_velocityBuffer;
    std::unique_ptr<VulkanBuffer> m_pressureBuffer;
    std::unique_ptr<VulkanBuffer> m_uniformBuffer;

    // Grid dimensions
    int m_gridSize = 64;
    uint64_t m_bufferSize = 0;
};

} // namespace ks