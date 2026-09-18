#include "ComputePipeline.h"
#include "VulkanFunctions.h"
#include "VulkanShaderLoader.h"
#include <QFile>
#include <QByteArray>
#include <QDebug>

namespace ks {

ComputePipeline::ComputePipeline(VulkanRenderer* renderer)
    : m_renderer(renderer)
{
}

ComputePipeline::~ComputePipeline() {
    destroy();
}

bool ComputePipeline::initialize(const QString& shaderPath) {
    if (!m_renderer || !m_renderer->vkFunctionsLoaded()) {
        qWarning() << "ComputePipeline: Renderer not properly initialized";
        return false;
    }

    VkDevice device = m_renderer->device();

    // Load the compute shader
    if (!m_shaderLoader || !m_renderer->shaderLoader()->loadShader("compute_pipeline", VK_SHADER_STAGE_COMPUTE_BIT, shaderPath)) {
        qWarning() << "Failed to load compute shader from" << shaderPath;
        return false;
    }

    // Create descriptor set layout based on pure virtual method
    QVector<VkDescriptorSetLayoutBinding> bindings = getDescriptorBindings();

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.constData();

    VkResult result = g_vk.createDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute descriptor set layout:" << result;
        return false;
    }

    m_pipelineLayout = VK_NULL_HANDLE;
    result = g_vk.createPipelineLayout(device, &VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout
    }, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute pipeline layout:" << result;
        return false;
    }

    // Create descriptor pool - size based on number of bindings
    uint32_t maxSets = 1;
    for (const auto& binding : bindings) {
        if (binding.descriptorCount > maxSets) {
            maxSets = binding.descriptorCount;
        }
    }

    VkDescriptorPoolSize poolSizes[8] = { };
    uint32_t poolSizeCount = 0;

    // Count descriptor types used
    uint32_t typeCount[32] = { };
    for (const auto& binding : bindings) {
        typeCount[binding.descriptorType]++;
    }

    for (uint32_t i = 0; i < 32; i++) {
        if (typeCount[i] > 0) {
            poolSizes[poolSizeCount].type = static_cast<VkDescriptorType>(i);
            poolSizes[poolSizeCount].descriptorCount = typeCount[i] * maxSets;
            poolSizeCount++;
        }
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizeCount;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = maxSets;

    result = g_vk.createDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute descriptor pool:" << result;
        return false;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    result = g_vk.allocateDescriptorSets(device, &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to allocate compute descriptor set:" << result;
        return false;
    }

    // Write descriptor set bindings
    for (uint32_t i = 0; i < bindings.size(); i++) {
        const auto& binding = bindings[i];
        VkDescriptorBufferInfo bufferInfo{};
        VkDescriptorImageInfo imageInfo{};

        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            // Find buffer binding
            auto it = m_bufferBindings.find(i);
            if (it != m_bufferBindings.end()) {
                bufferInfo.buffer = it->second.buffer;
                bufferInfo.offset = it->second.offset;
                bufferInfo.range = it->second.size;
            }
        } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            auto it = m_bufferBindings.find(i);
            if (it != m_bufferBindings.end()) {
                bufferInfo.buffer = it->second.buffer;
                bufferInfo.offset = it->second.offset;
                bufferInfo.range = it->second.size;
            }
        } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                   binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            auto it = m_imageBindings.find(i);
            if (it != m_imageBindings.end()) {
                imageInfo.view = it->second.view;
                imageInfo.sampler = it->second.sampler;
            }
        }

        g_vk.updateDescriptorSets(device, 1, &VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSet,
            .dstBinding = binding.binding,
            .descriptorCount = binding.descriptorCount,
            .descriptorType = binding.descriptorType,
            .pBufferInfo = binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                         binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                         ? &bufferInfo : nullptr,
            .pImageInfo = binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                         binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                         ? &imageInfo : nullptr
        }, nullptr);
    }

    // Create the compute pipeline
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = m_renderer->shaderLoader()->shaderModule("compute_pipeline")->m_module;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_pipelineLayout;

    VkPipeline computePipeline;
    result = g_vk.createComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute pipeline:" << result;
        return false;
    }
    m_pipeline = computePipeline;

    qInfo() << "Compute Pipeline initialized successfully";
    return true;
}

void ComputePipeline::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_pipeline != VK_NULL_HANDLE) {
            g_vk.destroyPipeline(m_device, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        g_vk.destroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        g_vk.destroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (m_descriptorPool != VK_NULL_HANDLE) {
        g_vk.destroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    if (m_descriptorSet != VK_NULL_HANDLE) {
        g_vk.freeDescriptorSets(m_device, m_descriptorPool, 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }
}

void ComputePipeline::setBufferBinding(uint32_t binding, VkBuffer buffer, VkDeviceSize size,
                                      VkDescriptorType type) {
    m_bufferBindings[binding] = { type, buffer, size };
}

void ComputePipeline::setImageBinding(uint32_t binding, VkImageView view,
                                      VkDescriptorType type,
                                      VkSampler sampler) {
    m_imageBindings[binding] = { type, view, sampler };
}

void ComputePipeline::dispatch(VkCommandBuffer cmd, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) {
    if (!m_pipeline || !m_device) return;

    g_vk.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    g_vk.cmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    g_vk.cmdDispatch(cmd, groupsX, groupsY, groupsZ);
    g_vk.cmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT, nullptr, nullptr, nullptr);
}
} // namespace ks