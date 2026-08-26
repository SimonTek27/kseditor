#include "VulkanComputePipeline.h"
#include "VulkanFunctions.h"
#include "VulkanShaderLoader.h"
#include <QFile>
#include <QByteArray>
#include <QDebug>

namespace ks {

VulkanComputePipeline::VulkanComputePipeline(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue computeQueue)
    : m_device(device), m_computeQueue(computeQueue)
{
}

VulkanComputePipeline::~VulkanComputePipeline() {
    destroy();
}

bool VulkanComputePipeline::initialize(const QString& computeShaderPath) {
    // Load the compute shader
    if (!m_shaderLoader || !m_shaderLoader->loadShader("cfd_compute", VK_SHADER_STAGE_COMPUTE_BIT, computeShaderPath)) {
        qWarning() << "Failed to load CFD compute shader";
        return false;
    }

    // Create descriptor set layout for compute shader
    QVector<VkDescriptorSetLayoutBinding> bindings;

    // Uniform buffer binding
    VkDescriptorSetLayoutBinding uniformBinding{};
    uniformBinding.binding = 0;
    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBinding.descriptorCount = 1;
    uniformBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.append(uniformBinding);

    // Storage buffers for velocity and pressure fields
    VkDescriptorSetLayoutBinding velBinding{};
    velBinding.binding = 1;
    velBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    velBinding.descriptorCount = 1;
    velBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.append(velBinding);

    VkDescriptorSetLayoutBinding presBinding{};
    presBinding.binding = 2;
    presBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    presBinding.descriptorCount = 1;
    presBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings.append(presBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.constData();

    VkResult result = g_vk.createDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute descriptor set layout:" << result;
        return false;
    }

    m_pipelineLayout = VK_NULL_HANDLE;
    result = g_vk.createPipelineLayout(m_device, &VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout
    }, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute pipeline layout:" << result;
        return false;
    }

    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[3] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2}
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    result = g_vk.createDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
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

    result = g_vk.allocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to allocate compute descriptor set:" << result;
        return false;
    }

    // Create uniform buffer
    m_uniformBuffer = std::make_unique<VulkanBuffer>(VulkanBuffer::UniformBuffer, this);
    m_uniformBuffer->setDevice(m_device, /*physicalDevice=*/VK_NULL_HANDLE);
    m_uniformBuffer->allocate(sizeof(QMatrix4x4) * 3 + sizeof(Uniforms));  // projection, view, model + uniforms
    // Map and bind will be done per-dispatch

    // Create storage buffers for velocity and pressure fields
    // 64^3 grid = 262144 cells
    m_gridSize = 64;
    m_bufferSize = uint64_t(m_gridSize) * m_gridSize * m_gridSize;

    m_velocityBuffer = std::make_unique<VulkanBuffer>(VulkanBuffer::StorageBuffer, this);
    m_velocityBuffer->setDevice(m_device, VK_NULL_HANDLE);
    m_velocityBuffer->allocate(m_bufferSize * sizeof(QVector3D), nullptr);

    m_pressureBuffer = std::make_unique<VulkanBuffer>(VulkanBuffer::StorageBuffer, this);
    m_pressureBuffer->setDevice(m_device, VK_NULL_HANDLE);
    m_pressureBuffer->allocate(m_bufferSize * sizeof(float), nullptr);

    // Write descriptor set bindings
    // Binding 0: Uniform buffer
    VkDescriptorBufferInfo uniformInfo{};
    uniformInfo.buffer = m_uniformBuffer->buffer();
    uniformInfo.offset = 0;
    uniformInfo.range = sizeof(QMatrix4x4) * 3 + sizeof(Uniforms);
    g_vk.updateDescriptorSets(m_device, 1, &VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &uniformInfo
    }, nullptr);

    // Binding 1: Velocity storage buffer
    VkDescriptorBufferInfo velInfo{};
    velInfo.buffer = m_velocityBuffer->buffer();
    velInfo.offset = 0;
    velInfo.range = m_bufferSize * sizeof(QVector3D);
    g_vk.updateDescriptorSets(m_device, 1, &VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSet,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &velInfo
    }, nullptr);

    // Binding 2: Pressure storage buffer
    VkDescriptorBufferInfo presInfo{};
    presInfo.buffer = m_pressureBuffer->buffer();
    presInfo.offset = 0;
    presInfo.range = m_bufferSize * sizeof(float);
    g_vk.updateDescriptorSets(m_device, 1, &VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSet,
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &presInfo
    }, nullptr);

    // Create the compute pipeline
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = m_shaderLoader->shaderModule("cfd_compute")->m_module;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = m_pipelineLayout;

    VkPipeline computePipeline;
    result = g_vk.createComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
    if (result != VK_SUCCESS) {
        qWarning() << "Failed to create compute pipeline:" << result;
        return false;
    }
    m_computePipeline = computePipeline;

    qInfo() << "CFD Compute Pipeline initialized successfully";
    return true;
}

void VulkanComputePipeline::destroy() {
    if (m_computePipeline != VK_NULL_HANDLE) {
        g_vk.destroyComputePipeline(m_device, m_computePipeline, nullptr);
        m_computePipeline = VK_NULL_HANDLE;
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
    m_velocityBuffer.reset();
    m_pressureBuffer.reset();
    m_uniformBuffer.reset();
}

void VulkanComputePipeline::dispatch(int workGroupX, int workGroupY, int workGroupZ) {
    if (!m_computePipeline || !m_device) return;

    vkCmdBindPipeline(vkGetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    vkCmdBindDescriptorSets(vkGetCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    vkDispatch(vkGetCurrentCommandBuffer(), workGroupX, workGroupY, workGroupZ);
    vkDispatchBarrier(vkGetCurrentCommandBuffer());
}

void VulkanComputePipeline::setUniform(const QString& name, const QVariant& value) {
    // Update uniform buffer data
    if (m_uniformBuffer && name == "projection") {
        QMatrix4x4* data = static_cast<QMatrix4x4*>(m_uniformBuffer->map());
        if (data) {
            *data = value.value<QMatrix4x4>();
            m_uniformBuffer->unmap();
        }
    }
    // Similar for other uniforms...
}

QVector<float> VulkanComputePipeline::readBackResults(int width, int height) {
    // Read back pressure field for visualization
    // This would use a staging buffer and vkMapMemory
    Q_UNUSED(width);
    Q_UNUSED(height);
    return QVector<float>();
}

} // namespace ks