#pragma once

#include <vulkan/vulkan.h>
#include <QVector>
#include <QMap>
#include <QString>
#include <memory>

namespace ks {

class VulkanRenderer;

class ComputePipeline {
public:
    ComputePipeline(VulkanRenderer* renderer);
    virtual ~ComputePipeline();

    bool initialize(const QString& shaderPath);
    void destroy();

    void setBufferBinding(uint32_t binding, VkBuffer buffer, VkDeviceSize size,
                         VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    void setImageBinding(uint32_t binding, VkImageView view,
                        VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                        VkSampler sampler = VK_NULL_HANDLE);

    void dispatch(VkCommandBuffer cmd, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);

    VkPipeline pipeline() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }
    VkDescriptorSet descriptorSet() const { return m_descriptorSet; }

protected:
    virtual QVector<VkDescriptorSetLayoutBinding> getDescriptorBindings() const = 0;

    VulkanRenderer* m_renderer = nullptr;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    struct BufferBindingInfo {
        VkDescriptorType type;
        VkBuffer buffer;
        VkDeviceSize size;
    };
    struct ImageBindingInfo {
        VkDescriptorType type;
        VkImageView view;
        VkSampler sampler;
    };
    QMap<uint32_t, BufferBindingInfo> m_bufferBindings;
    QMap<uint32_t, ImageBindingInfo> m_imageBindings;
};

} // namespace ks