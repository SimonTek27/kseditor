#include "NativeRenderer.h"
#include "ShadowSystem.h"
#include "ui/UiGpuPass.h"
#include "ui/UiRenderer.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cstddef>

// No Qt anywhere in this file. Unlike ks::VulkanRenderer::createDevice()
// (whose entire body is wrapped in `#if QT_CONFIG(vulkan)` and calls into a
// QLibrary-loaded function table), this links directly against the Vulkan
// loader — the same way SimulatorApp.cpp already creates its VkInstance and
// Win32 surface.

namespace ks::sim {

namespace {

uint32_t findMemoryType(VkPhysicalDevice pd, uint32_t typeBits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(pd, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props) return i;
    }
    return 0;
}

std::vector<char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open()) return {};
    size_t size = static_cast<size_t>(f.tellg());
    std::vector<char> data(size);
    f.seekg(0);
    f.read(data.data(), static_cast<std::streamsize>(size));
    return data;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule mod = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &ci, nullptr, &mod);
    return mod;
}

bool createBuffer(VkPhysicalDevice pd, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size = size;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &ci, nullptr, &buffer) != VK_SUCCESS) return false;

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(device, buffer, &mr);
    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = findMemoryType(pd, mr.memoryTypeBits, props);
    if (vkAllocateMemory(device, &mai, nullptr, &memory) != VK_SUCCESS) return false;
    vkBindBufferMemory(device, buffer, memory, 0);
    return true;
}

} // namespace

NativeRenderer::~NativeRenderer() { shutdown(); }

bool NativeRenderer::createDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) { fprintf(stderr, "[NativeRenderer] no Vulkan physical devices\n"); return false; }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    m_physicalDevice = devices[0];
    for (auto dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { m_physicalDevice = dev; break; }
    }

    uint32_t qfCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &qfCount, nullptr);
    std::vector<VkQueueFamilyProperties> qFamilies(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &qfCount, qFamilies.data());

    bool found = false;
    for (uint32_t i = 0; i < qfCount; ++i) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, surface, &presentSupport);
        if ((qFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            m_graphicsQueueFamily = i;
            found = true;
            break;
        }
    }
    if (!found) { fprintf(stderr, "[NativeRenderer] no graphics+present queue family\n"); return false; }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qCi{};
    qCi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qCi.queueFamilyIndex = m_graphicsQueueFamily;
    qCi.queueCount = 1;
    qCi.pQueuePriorities = &priority;

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo devCi{};
    devCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devCi.queueCreateInfoCount = 1;
    devCi.pQueueCreateInfos = &qCi;
    devCi.enabledExtensionCount = 1;
    devCi.ppEnabledExtensionNames = extensions;
    devCi.pEnabledFeatures = &features;

    if (vkCreateDevice(m_physicalDevice, &devCi, nullptr, &m_device) != VK_SUCCESS) {
        fprintf(stderr, "[NativeRenderer] vkCreateDevice failed\n");
        return false;
    }
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

    return createCommandPoolAndBuffer() && createSyncObjects();
}

bool NativeRenderer::createCommandPoolAndBuffer() {
    VkCommandPoolCreateInfo poolCi{};
    poolCi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCi.queueFamilyIndex = m_graphicsQueueFamily;
    poolCi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(m_device, &poolCi, nullptr, &m_commandPool) != VK_SUCCESS) return false;

    VkCommandBufferAllocateInfo cbAi{};
    cbAi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAi.commandPool = m_commandPool;
    cbAi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAi.commandBufferCount = 1;
    return vkAllocateCommandBuffers(m_device, &cbAi, &m_commandBuffer) == VK_SUCCESS;
}

bool NativeRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo semCi{};
    semCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceCi{};
    fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    return vkCreateSemaphore(m_device, &semCi, nullptr, &m_imageAvailable) == VK_SUCCESS &&
           vkCreateSemaphore(m_device, &semCi, nullptr, &m_renderFinished) == VK_SUCCESS &&
           vkCreateFence(m_device, &fenceCi, nullptr, &m_inFlightFence) == VK_SUCCESS;
}

bool NativeRenderer::createDepthResources(uint32_t width, uint32_t height) {
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent = {width, height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.format = VK_FORMAT_D32_SFLOAT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(m_device, &ci, nullptr, &m_depthImage) != VK_SUCCESS) return false;

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(m_device, m_depthImage, &mr);
    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = findMemoryType(m_physicalDevice, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(m_device, &mai, nullptr, &m_depthMemory) != VK_SUCCESS) return false;
    vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);

    VkImageViewCreateInfo vCi{};
    vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vCi.image = m_depthImage;
    vCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vCi.format = VK_FORMAT_D32_SFLOAT;
    vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vCi.subresourceRange.levelCount = 1;
    vCi.subresourceRange.layerCount = 1;
    return vkCreateImageView(m_device, &vCi, nullptr, &m_depthView) == VK_SUCCESS;
}

void NativeRenderer::destroySwapChain() {
    if (!m_device) return;
    // Extent-bound and swapchain-format-bound, so it has to go with the
    // swapchain; the next endFrame() with deferred on rebuilds it lazily.
    destroyDeferredResources();
    for (auto fb : m_swapChainFramebuffers) if (fb) vkDestroyFramebuffer(m_device, fb, nullptr);
    m_swapChainFramebuffers.clear();
    for (auto view : m_swapChainImageViews) if (view) vkDestroyImageView(m_device, view, nullptr);
    m_swapChainImageViews.clear();
    m_swapChainImages.clear();
    if (m_depthView) { vkDestroyImageView(m_device, m_depthView, nullptr); m_depthView = VK_NULL_HANDLE; }
    if (m_depthImage) { vkDestroyImage(m_device, m_depthImage, nullptr); m_depthImage = VK_NULL_HANDLE; }
    if (m_depthMemory) { vkFreeMemory(m_device, m_depthMemory, nullptr); m_depthMemory = VK_NULL_HANDLE; }
    if (m_renderPass) { vkDestroyRenderPass(m_device, m_renderPass, nullptr); m_renderPass = VK_NULL_HANDLE; }
    if (m_swapChain) { vkDestroySwapchainKHR(m_device, m_swapChain, nullptr); m_swapChain = VK_NULL_HANDLE; }
}

bool NativeRenderer::createSwapChain(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    destroySwapChain();

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, formats.data());
    VkSurfaceFormatKHR chosen = formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { chosen = f; break; }
    }
    m_swapChainFormat = chosen.format;

    VkExtent2D extent = caps.currentExtent.width != UINT32_MAX
        ? caps.currentExtent
        : VkExtent2D{std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width),
                     std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height)};
    if (extent.width == 0 || extent.height == 0) return false;
    m_swapChainExtent = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sCi{};
    sCi.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sCi.surface = surface;
    sCi.minImageCount = imageCount;
    sCi.imageFormat = chosen.format;
    sCi.imageColorSpace = chosen.colorSpace;
    sCi.imageExtent = extent;
    sCi.imageArrayLayers = 1;
    sCi.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sCi.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sCi.preTransform = caps.currentTransform;
    sCi.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sCi.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync; always supported
    sCi.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &sCi, nullptr, &m_swapChain) != VK_SUCCESS) {
        fprintf(stderr, "[NativeRenderer] vkCreateSwapchainKHR failed\n");
        return false;
    }

    uint32_t actualCount = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &actualCount, nullptr);
    m_swapChainImages.resize(actualCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &actualCount, m_swapChainImages.data());

    m_swapChainImageViews.resize(actualCount);
    for (uint32_t i = 0; i < actualCount; ++i) {
        VkImageViewCreateInfo vCi{};
        vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vCi.image = m_swapChainImages[i];
        vCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vCi.format = m_swapChainFormat;
        vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vCi.subresourceRange.levelCount = 1;
        vCi.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device, &vCi, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) return false;
    }

    if (!createDepthResources(extent.width, extent.height)) return false;

    VkAttachmentDescription colorAtt{};
    colorAtt.format = m_swapChainFormat;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAtt{};
    depthAtt.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription atts[2] = {colorAtt, depthAtt};
    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpCi{};
    rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCi.attachmentCount = 2;
    rpCi.pAttachments = atts;
    rpCi.subpassCount = 1;
    rpCi.pSubpasses = &subpass;
    rpCi.dependencyCount = 1;
    rpCi.pDependencies = &dep;
    if (vkCreateRenderPass(m_device, &rpCi, nullptr, &m_renderPass) != VK_SUCCESS) return false;

    m_swapChainFramebuffers.resize(actualCount);
    for (uint32_t i = 0; i < actualCount; ++i) {
        VkImageView fbAtts[2] = {m_swapChainImageViews[i], m_depthView};
        VkFramebufferCreateInfo fbCi{};
        fbCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCi.renderPass = m_renderPass;
        fbCi.attachmentCount = 2;
        fbCi.pAttachments = fbAtts;
        fbCi.width = extent.width;
        fbCi.height = extent.height;
        fbCi.layers = 1;
        if (vkCreateFramebuffer(m_device, &fbCi, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) return false;
    }

    return true;
}

bool NativeRenderer::recreateSwapChain(uint32_t width, uint32_t height) {
    if (m_device) vkDeviceWaitIdle(m_device);
    // Caller (SimulatorApp) owns the VkSurfaceKHR; recreateSwapChain here
    // assumes createSwapChain() was already called once with it, since this
    // class doesn't store the surface handle itself (SimulatorApp does).
    // In practice call createSwapChain() again with the stored surface from
    // the WM_SIZE handler instead of this convenience wrapper if that
    // ever becomes awkward.
    fprintf(stderr, "[NativeRenderer] recreateSwapChain: call createSwapChain(surface, %u, %u) directly from the resize handler\n", width, height);
    return false;
}

bool NativeRenderer::loadPipelines(const std::string& shaderDir) {
    m_shaderDir = shaderDir;
    m_vertModule = createShaderModule(m_device, readFile(shaderDir + "/native_forward.vert.spv"));
    m_fragModule = createShaderModule(m_device, readFile(shaderDir + "/native_forward.frag.spv"));
    if (!m_vertModule || !m_fragModule) {
        fprintf(stderr, "[NativeRenderer] failed to load native_forward.{vert,frag}.spv from %s\n", shaderDir.c_str());
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = m_vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = m_fragModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{0, sizeof(NativeVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[4] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NativeVertex, px)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NativeVertex, nx)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(NativeVertex, u)},
        {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(NativeVertex, r)},
    };
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 4;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAtt;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkDynamicState dynStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = 2;
    dynState.pDynamicStates = dynStates;

    // Descriptor set 0: frame-global UBO (light + cascade matrices) + shadow
    // cascade array sampler — the persistent layout/pool/set/UBO/dummy
    // texture are created here and written each frame in endFrame().
    if (!createFrameDescriptorResources()) {
        fprintf(stderr, "[NativeRenderer] failed to create frame descriptor resources\n");
        return false;
    }

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.size = sizeof(float) * 16 * 2; // model + mvp

    VkPipelineLayoutCreateInfo layoutCi{};
    layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCi.setLayoutCount = 1;
    layoutCi.pSetLayouts = &m_frameSetLayout;
    layoutCi.pushConstantRangeCount = 1;
    layoutCi.pPushConstantRanges = &pcRange;
    if (vkCreatePipelineLayout(m_device, &layoutCi, nullptr, &m_pipelineLayout) != VK_SUCCESS) return false;

    VkGraphicsPipelineCreateInfo pipeCi{};
    pipeCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCi.stageCount = 2;
    pipeCi.pStages = stages;
    pipeCi.pVertexInputState = &vertexInput;
    pipeCi.pInputAssemblyState = &inputAssembly;
    pipeCi.pViewportState = &viewportState;
    pipeCi.pRasterizationState = &rasterizer;
    pipeCi.pMultisampleState = &multisampling;
    pipeCi.pColorBlendState = &colorBlend;
    pipeCi.pDepthStencilState = &depthStencil;
    pipeCi.pDynamicState = &dynState;
    pipeCi.layout = m_pipelineLayout;
    pipeCi.renderPass = m_renderPass;
    pipeCi.subpass = 0;

    return vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeCi, nullptr, &m_pipeline) == VK_SUCCESS;
}

// Layout must match the `FrameData` uniform block declared in
// native_forward.frag, deferred_lighting.frag and taa.frag. std140 puts the
// first five members at 0..256 exactly as before; viewProj/prevViewProj/
// taaParams are appended after them, so the forward shader — whose block
// simply stops at cameraPos — still sees byte-identical offsets for every
// member it declares (Vulkan only requires the buffer range to cover the
// shader's block, not to match it).
struct FrameDataUBO {
    float sunDirection[4];
    float sunColor[4];
    float cascadeViewProj[3][16];
    float cascadeSplits[4];
    float cameraPos[4];
    float viewProj[16];       // unjittered, current frame
    float prevViewProj[16];   // unjittered, previous frame
    float taaParams[4];       // x = history feedback, 0 = no temporal blend
};
static_assert(sizeof(FrameDataUBO) == 400, "FrameDataUBO is read as a std140 uniform block");

// Halton low-discrepancy sequence — the jitter source for TAA. Eight samples
// gives a dense enough sub-pixel pattern that edges resolve without the
// visible 2x2 grid a naive checkerboard jitter produces.
static float halton(int index, int base) {
    float f = 1.0f, r = 0.0f;
    while (index > 0) {
        f /= static_cast<float>(base);
        r += f * static_cast<float>(index % base);
        index /= base;
    }
    return r;
}

// Full-resolution colour target that is both rendered into and sampled back
// out — used for the deferred lighting output and the two TAA history slots.
static bool createSampledTarget(VkPhysicalDevice pd, VkDevice device, uint32_t width, uint32_t height,
                                VkFormat format, VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = format;
    ci.extent = {width, height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &ci, nullptr, &image) != VK_SUCCESS) return false;

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(device, image, &mr);
    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = findMemoryType(pd, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &mai, nullptr, &memory) != VK_SUCCESS) return false;
    vkBindImageMemory(device, image, memory, 0);

    VkImageViewCreateInfo vCi{};
    vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vCi.image = image;
    vCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vCi.format = format;
    vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vCi.subresourceRange.levelCount = 1;
    vCi.subresourceRange.layerCount = 1;
    return vkCreateImageView(device, &vCi, nullptr, &view) == VK_SUCCESS;
}

bool NativeRenderer::createDummyShadowTexture() {
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent = {1, 1, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 3; // matches sampler2DArray indexing (cascade 0..2)
    ci.format = VK_FORMAT_R32_SFLOAT;
    ci.tiling = VK_IMAGE_TILING_LINEAR; // host-writable without a staging buffer, fine for a 1x1x3 dummy
    ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    if (vkCreateImage(m_device, &ci, nullptr, &m_dummyShadowImage) != VK_SUCCESS) return false;

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(m_device, m_dummyShadowImage, &mr);
    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = findMemoryType(m_physicalDevice, mr.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(m_device, &mai, nullptr, &m_dummyShadowMemory) != VK_SUCCESS) return false;
    vkBindImageMemory(m_device, m_dummyShadowImage, m_dummyShadowMemory, 0);

    // Fill every layer with 1.0 (max NDC depth => sampleShadow() in
    // native_forward.frag never finds the fragment "in front of" this, so
    // the dummy always reads as fully lit).
    void* mapped = nullptr;
    vkMapMemory(m_device, m_dummyShadowMemory, 0, mr.size, 0, &mapped);
    if (mapped) {
        float* f = reinterpret_cast<float*>(mapped);
        for (VkDeviceSize i = 0; i < mr.size / sizeof(float); ++i) f[i] = 1.0f;
        vkUnmapMemory(m_device, m_dummyShadowMemory);
    }

    VkImageViewCreateInfo vCi{};
    vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vCi.image = m_dummyShadowImage;
    vCi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    vCi.format = VK_FORMAT_R32_SFLOAT;
    vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vCi.subresourceRange.levelCount = 1;
    vCi.subresourceRange.layerCount = 3;
    if (vkCreateImageView(m_device, &vCi, nullptr, &m_dummyShadowView) != VK_SUCCESS) return false;

    VkSamplerCreateInfo sCi{};
    sCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sCi.magFilter = VK_FILTER_NEAREST;
    sCi.minFilter = VK_FILTER_NEAREST;
    sCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sCi.maxLod = 1.0f;
    return vkCreateSampler(m_device, &sCi, nullptr, &m_dummyShadowSampler) == VK_SUCCESS;
}

bool NativeRenderer::createFrameDescriptorResources() {
    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslCi{};
    dslCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslCi.bindingCount = 2;
    dslCi.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(m_device, &dslCi, nullptr, &m_frameSetLayout) != VK_SUCCESS) return false;

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCi{};
    poolCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCi.maxSets = 1;
    poolCi.poolSizeCount = 2;
    poolCi.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(m_device, &poolCi, nullptr, &m_descriptorPool) != VK_SUCCESS) return false;

    VkDescriptorSetAllocateInfo dsAi{};
    dsAi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAi.descriptorPool = m_descriptorPool;
    dsAi.descriptorSetCount = 1;
    dsAi.pSetLayouts = &m_frameSetLayout;
    if (vkAllocateDescriptorSets(m_device, &dsAi, &m_frameSet) != VK_SUCCESS) return false;

    if (!createBuffer(m_physicalDevice, m_device, sizeof(FrameDataUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_frameUBO, m_frameUBOMemory)) {
        return false;
    }
    vkMapMemory(m_device, m_frameUBOMemory, 0, sizeof(FrameDataUBO), 0, &m_frameUBOMapped);

    if (!createDummyShadowTexture()) return false;

    // Bind the dummy shadow texture initially; writeFrameDescriptorSet() is
    // called again with the real cascade view once a CascadedShadowMap is
    // attached and initialized (see endFrame()).
    writeFrameDescriptorSet(m_dummyShadowView, m_dummyShadowSampler);
    return true;
}

void NativeRenderer::writeFrameDescriptorSet(VkImageView shadowView, VkSampler shadowSampler) {
    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = m_frameUBO;
    bufInfo.range = sizeof(FrameDataUBO);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.sampler = shadowSampler;
    imgInfo.imageView = shadowView;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // The dummy texture is left in VK_IMAGE_LAYOUT_PREINITIALIZED (never
    // transitioned, since it's written directly via a host-mapped linear
    // image and read once here) — SHADER_READ_ONLY_OPTIMAL is technically
    // incorrect for that specific case on some drivers; PREINITIALIZED/
    // GENERAL is the layout that actually matches a linear-tiled, never-
    // transitioned image. Use GENERAL for correctness across drivers.
    if (shadowView == m_dummyShadowView) imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[2]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_frameSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufInfo;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_frameSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);
}

void NativeRenderer::uploadMesh(NativeMesh& mesh) {
    VkDeviceSize vSize = sizeof(NativeVertex) * mesh.vertices.size();
    VkDeviceSize iSize = sizeof(uint32_t) * mesh.indices.size();

    createBuffer(m_physicalDevice, m_device, vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mesh.vertexBuffer, mesh.vertexMemory);
    void* vData = nullptr;
    vkMapMemory(m_device, mesh.vertexMemory, 0, vSize, 0, &vData);
    std::memcpy(vData, mesh.vertices.data(), static_cast<size_t>(vSize));
    vkUnmapMemory(m_device, mesh.vertexMemory);

    if (!mesh.indices.empty()) {
        createBuffer(m_physicalDevice, m_device, iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    mesh.indexBuffer, mesh.indexMemory);
        void* iData = nullptr;
        vkMapMemory(m_device, mesh.indexMemory, 0, iSize, 0, &iData);
        std::memcpy(iData, mesh.indices.data(), static_cast<size_t>(iSize));
        vkUnmapMemory(m_device, mesh.indexMemory);
    }
    // NOTE: host-visible/coherent buffers, not device-local + staged. Simpler
    // and correct; revisit for perf once real car/track meshes (tens of
    // thousands of verts) are flowing through here instead of test geometry.
}

bool NativeRenderer::loadMeshFromFile(const std::string& name, const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) { fprintf(stderr, "[NativeRenderer] cannot open mesh cache %s\n", path.c_str()); return false; }

    char magic[4];
    f.read(magic, 4);
    if (std::memcmp(magic, "NMSH", 4) != 0) { fprintf(stderr, "[NativeRenderer] bad mesh cache magic in %s\n", path.c_str()); return false; }

    uint32_t vCount = 0, iCount = 0;
    f.read(reinterpret_cast<char*>(&vCount), 4);
    f.read(reinterpret_cast<char*>(&iCount), 4);

    NativeMesh mesh;
    mesh.vertices.resize(vCount);
    mesh.indices.resize(iCount);
    f.read(reinterpret_cast<char*>(mesh.vertices.data()), static_cast<std::streamsize>(sizeof(NativeVertex) * vCount));
    f.read(reinterpret_cast<char*>(mesh.indices.data()), static_cast<std::streamsize>(sizeof(uint32_t) * iCount));
    if (!f) { fprintf(stderr, "[NativeRenderer] truncated mesh cache %s\n", path.c_str()); return false; }

    setMesh(name, mesh);
    return true;
}

int NativeRenderer::loadMeshesFromManifest(const std::string& dir) {
    std::ifstream manifest(dir + "/manifest.txt");
    if (!manifest.is_open()) {
        fprintf(stderr, "[NativeRenderer] no manifest.txt in %s\n", dir.c_str());
        return 0;
    }
    int loaded = 0;
    std::string name;
    while (std::getline(manifest, name)) {
        if (name.empty()) continue;
        if (loadMeshFromFile(name, dir + "/" + name + ".nmsh")) {
            m_staticSceneMeshNames.push_back(name);
            ++loaded;
        }
    }
    return loaded;
}

void NativeRenderer::drawStaticScene() {
    for (const auto& name : m_staticSceneMeshNames) {
        drawMesh(name, mat4());
    }
}

void NativeRenderer::setMesh(const std::string& name, const NativeMesh& meshIn) {
    destroyMesh(name);
    NativeMesh mesh = meshIn;
    uploadMesh(mesh);
    m_meshes[name] = mesh;
}

void NativeRenderer::destroyMesh(const std::string& name) {
    auto it = m_meshes.find(name);
    if (it == m_meshes.end()) return;
    if (it->second.vertexBuffer) vkDestroyBuffer(m_device, it->second.vertexBuffer, nullptr);
    if (it->second.vertexMemory) vkFreeMemory(m_device, it->second.vertexMemory, nullptr);
    if (it->second.indexBuffer) vkDestroyBuffer(m_device, it->second.indexBuffer, nullptr);
    if (it->second.indexMemory) vkFreeMemory(m_device, it->second.indexMemory, nullptr);
    m_meshes.erase(it);
    m_staticSceneMeshNames.erase(
        std::remove(m_staticSceneMeshNames.begin(), m_staticSceneMeshNames.end(), name),
        m_staticSceneMeshNames.end());
}

bool NativeRenderer::beginFrame() {
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult acquireResult = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                                   m_imageAvailable, VK_NULL_HANDLE, &m_currentImageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) return false; // caller should recreate the swapchain
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) return false;

    m_drawList.clear();
    return true;
}

void NativeRenderer::drawMesh(const std::string& name, const mat4& modelMatrix) {
    // Queued, not drawn immediately: the shadow cascades and the main color
    // pass both need to render this exact instance list (see endFrame()),
    // so recording once and replaying it twice is what makes cascade
    // shadows line up with what's actually on screen instead of assuming
    // every caster sits at the identity transform.
    if (m_meshes.find(name) == m_meshes.end()) return;
    m_drawList.push_back({name, modelMatrix});
}

void NativeRenderer::endFrame() {
    vkResetFences(m_device, 1, &m_inFlightFence);
    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    // ---- Shadow cascades: same draw list, light-space matrices instead of
    // the camera's view-projection. ----
    if (m_shadowMap && m_shadowMap->isInitialized()) {
        float camNear = 0.5f, camFar = 500.0f; // TODO: thread real near/far from CameraController instead of these defaults
        m_shadowMap->update(m_view, m_proj, camNear, camFar, m_sun.direction);
        for (int c = 0; c < m_shadowMap->cascadeCount(); ++c) {
            m_shadowMap->beginCascadePass(m_commandBuffer, c);
            for (const auto& draw : m_drawList) {
                auto it = m_meshes.find(draw.meshName);
                if (it == m_meshes.end() || !it->second.vertexBuffer) continue;
                const NativeMesh& mesh = it->second;
                VkBuffer vBufs[] = {mesh.vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vBufs, offsets);
                m_shadowMap->pushLightSpaceMatrix(m_commandBuffer, c, draw.model);
                if (mesh.indexBuffer) {
                    vkCmdBindIndexBuffer(m_commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(m_commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
                } else {
                    vkCmdDraw(m_commandBuffer, static_cast<uint32_t>(mesh.vertices.size()), 1, 0, 0);
                }
            }
            m_shadowMap->endCascadePass(m_commandBuffer);
        }
    }

    // Lazy build: the forward path above never pays for any of this, and a
    // failed build silently falls back to forward instead of going black.
    // TAA implies deferred — its resolve pass is the thing that reads the
    // GBuffer world positions back out for reprojection.
    const bool deferred = (m_deferred || m_taa) && ensureDeferredResources();
    const bool useTaa = deferred && m_taa;

    // Jitter only the main-pass projection. The shadow cascades above and
    // both reprojection matrices below stay unjittered, otherwise the motion
    // vectors would carry the jitter and TAA would smear instead of resolve.
    mat4 projRender = m_proj;
    if (useTaa && m_swapChainExtent.width > 0) {
        m_jitterIndex = (m_jitterIndex + 1) % kJitterSamples;
        const float w = static_cast<float>(m_swapChainExtent.width);
        const float h = static_cast<float>(m_swapChainExtent.height);
        // One pixel spans 2/NDC units, so (halton - 0.5) * 2/N is a uniform
        // half-pixel offset per axis.
        projRender(0, 2) += (halton(m_jitterIndex + 1, 2) - 0.5f) * 2.0f / w;
        projRender(1, 2) += (halton(m_jitterIndex + 1, 3) - 0.5f) * 2.0f / h;
    }

    const mat4 viewProjUnjit = m_proj * m_view;
    const mat4 prevViewProjUnjit = m_prevViewProjValid ? m_prevViewProj : viewProjUnjit;

    // ---- Frame-global lighting data, shared by whichever geometry pass
    // runs below (forward or GBuffer). ----
    FrameDataUBO frameData{};
    frameData.sunDirection[0] = m_sun.direction.x;
    frameData.sunDirection[1] = m_sun.direction.y;
    frameData.sunDirection[2] = m_sun.direction.z;
    frameData.sunColor[0] = m_sun.color.x;
    frameData.sunColor[1] = m_sun.color.y;
    frameData.sunColor[2] = m_sun.color.z;
    frameData.sunColor[3] = m_sun.intensity;
    frameData.cameraPos[0] = m_camPosWS.x;
    frameData.cameraPos[1] = m_camPosWS.y;
    frameData.cameraPos[2] = m_camPosWS.z;
    std::memcpy(frameData.viewProj, viewProjUnjit.data(), sizeof(float) * 16);
    std::memcpy(frameData.prevViewProj, prevViewProjUnjit.data(), sizeof(float) * 16);
    // 0 on the first frame after a (re)build, so undefined history can never
    // show up as a flash of garbage.
    frameData.taaParams[0] = (useTaa && m_historyValid) ? kTaaFeedback : 0.0f;

    // Either the real shadow cascade array or the 1x1 dummy (always-lit)
    // texture if no shadow map is attached/initialized yet.
    VkImageView shadowView = m_dummyShadowView;
    VkSampler shadowSampler = m_dummyShadowSampler;
    if (m_shadowMap && m_shadowMap->isInitialized()) {
        for (int c = 0; c < m_shadowMap->cascadeCount() && c < 3; ++c) {
            std::memcpy(frameData.cascadeViewProj[c], m_shadowMap->cascade(c).viewProj.data(), sizeof(float) * 16);
            frameData.cascadeSplits[c] = m_shadowMap->cascade(c).splitDepth;
        }
        shadowView = m_shadowMap->arrayView();
        shadowSampler = m_shadowMap->sampler();
    }
    writeFrameDescriptorSet(shadowView, shadowSampler);
    std::memcpy(m_frameUBOMapped, &frameData, sizeof(FrameDataUBO));

    // Replay of the queued instance list, shared by the forward and GBuffer
    // passes: both bind m_pipelineLayout (set0 = FrameData + shadow array,
    // push constants = model + mvp), so only the pipeline object differs.
    auto recordDrawList = [&]() {
        for (const auto& draw : m_drawList) {
            auto it = m_meshes.find(draw.meshName);
            if (it == m_meshes.end() || !it->second.vertexBuffer) continue;
            const NativeMesh& mesh = it->second;

            struct { mat4 model; mat4 mvp; } pc;
            pc.model = draw.model;
            pc.mvp = projRender * m_view * draw.model;
            vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

            VkBuffer vBufs[] = {mesh.vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vBufs, offsets);

            if (mesh.indexBuffer) {
                vkCmdBindIndexBuffer(m_commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(m_commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
            } else {
                vkCmdDraw(m_commandBuffer, static_cast<uint32_t>(mesh.vertices.size()), 1, 0, 0);
            }
        }
    };

    VkViewport vp{0, 0, float(m_swapChainExtent.width), float(m_swapChainExtent.height), 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, m_swapChainExtent};

    if (deferred) {
        // ---- Pass 1: GBuffer — geometry attributes into three colour
        // targets plus a private depth buffer (kept separate from
        // m_depthImage so the forward path's depth layout is untouched). ----
        VkClearValue gClears[4];
        for (int i = 0; i < 3; ++i) gClears[i].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        gClears[3].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo gBegin{};
        gBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        gBegin.renderPass = m_gbufferRenderPass;
        gBegin.framebuffer = m_gbufferFramebuffers[m_currentImageIndex];
        gBegin.renderArea.extent = m_swapChainExtent;
        gBegin.clearValueCount = 4;
        gBegin.pClearValues = gClears;
        vkCmdBeginRenderPass(m_commandBuffer, &gBegin, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(m_commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gbufferPipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                                0, 1, &m_frameSet, 0, nullptr);
        recordDrawList();
        vkCmdEndRenderPass(m_commandBuffer);

        // ---- Pass 2: fullscreen lighting + volumetric fog into an
        // offscreen HDR target, sampling the GBuffer written above. ----
        VkClearValue lClear{};
        lClear.color = {{0.35f, 0.55f, 0.75f, 1.0f}};

        VkRenderPassBeginInfo lBegin{};
        lBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        lBegin.renderPass = m_lightingRenderPass;
        lBegin.framebuffer = m_lightingFramebuffer;
        lBegin.renderArea.extent = m_swapChainExtent;
        lBegin.clearValueCount = 1;
        lBegin.pClearValues = &lClear;
        vkCmdBeginRenderPass(m_commandBuffer, &lBegin, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(m_commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
        writeLightingDescriptorSet(shadowView, shadowSampler);
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingLayout,
                                0, 1, &m_lightingSet, 0, nullptr);
        vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(m_commandBuffer);

        // ---- Pass 3: temporal resolve into the swapchain and into the
        // history slot the *next* frame reads. With TAA off this degenerates
        // to an exact HDR -> swapchain copy (feedback is 0 in the UBO).
        // Reading history[readIdx] while writing history[1 - readIdx] is what
        // keeps the two out of each other's way inside one subpass. ----
        const int historyWrite = 1 - m_historyIndex;
        const size_t swapCount = m_swapChainImageViews.size();

        VkClearValue rClears[2];
        rClears[0].color = {{0.35f, 0.55f, 0.75f, 1.0f}};
        rClears[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo rBegin{};
        rBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rBegin.renderPass = m_resolveRenderPass;
        rBegin.framebuffer = m_resolveFramebuffers[historyWrite * static_cast<int>(swapCount) + m_currentImageIndex];
        rBegin.renderArea.extent = m_swapChainExtent;
        rBegin.clearValueCount = 2;
        rBegin.pClearValues = rClears;
        vkCmdBeginRenderPass(m_commandBuffer, &rBegin, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(m_commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
        writeResolveDescriptorSet();
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolvePipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolveLayout,
                                0, 1, &m_resolveSet, 0, nullptr);
        vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(m_commandBuffer);
    } else {
        // ---- Main color pass, same draw list again. ----
        VkClearValue clears[2];
        clears[0].color = {{0.35f, 0.55f, 0.75f, 1.0f}};
        clears[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = m_renderPass;
        rpBegin.framebuffer = m_swapChainFramebuffers[m_currentImageIndex];
        rpBegin.renderArea.extent = m_swapChainExtent;
        rpBegin.clearValueCount = 2;
        rpBegin.pClearValues = clears;
        vkCmdBeginRenderPass(m_commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(m_commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                                0, 1, &m_frameSet, 0, nullptr);
        recordDrawList();
        vkCmdEndRenderPass(m_commandBuffer);
    }

    if (deferred) {
        m_historyValid = true;
        m_historyIndex ^= 1;
    }
    m_prevViewProj = viewProjUnjit;
    m_prevViewProjValid = true;

    vkEndCommandBuffer(m_commandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &m_imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &m_renderFinished;
    vkQueueSubmit(m_graphicsQueue, 1, &submit, m_inFlightFence);

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &m_renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapChain;
    present.pImageIndices = &m_currentImageIndex;
    vkQueuePresentKHR(m_graphicsQueue, &present);
}

void NativeRenderer::shutdown() {
    if (!m_device) return;
    vkDeviceWaitIdle(m_device);

    // Torn down first so no deferred pipeline outlives m_pipelineLayout.
    destroyDeferredResources();

    for (auto& [name, mesh] : m_meshes) {
        if (mesh.vertexBuffer) vkDestroyBuffer(m_device, mesh.vertexBuffer, nullptr);
        if (mesh.vertexMemory) vkFreeMemory(m_device, mesh.vertexMemory, nullptr);
        if (mesh.indexBuffer) vkDestroyBuffer(m_device, mesh.indexBuffer, nullptr);
        if (mesh.indexMemory) vkFreeMemory(m_device, mesh.indexMemory, nullptr);
    }
    m_meshes.clear();

    if (m_pipeline) vkDestroyPipeline(m_device, m_pipeline, nullptr);
    if (m_pipelineLayout) vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    if (m_vertModule) vkDestroyShaderModule(m_device, m_vertModule, nullptr);
    if (m_fragModule) vkDestroyShaderModule(m_device, m_fragModule, nullptr);

    if (m_frameUBOMapped) { vkUnmapMemory(m_device, m_frameUBOMemory); m_frameUBOMapped = nullptr; }
    if (m_frameUBO) vkDestroyBuffer(m_device, m_frameUBO, nullptr);
    if (m_frameUBOMemory) vkFreeMemory(m_device, m_frameUBOMemory, nullptr);
    if (m_descriptorPool) vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr); // also frees m_frameSet
    if (m_frameSetLayout) vkDestroyDescriptorSetLayout(m_device, m_frameSetLayout, nullptr);
    if (m_dummyShadowSampler) vkDestroySampler(m_device, m_dummyShadowSampler, nullptr);
    if (m_dummyShadowView) vkDestroyImageView(m_device, m_dummyShadowView, nullptr);
    if (m_dummyShadowImage) vkDestroyImage(m_device, m_dummyShadowImage, nullptr);
    if (m_dummyShadowMemory) vkFreeMemory(m_device, m_dummyShadowMemory, nullptr);

    destroySwapChain();

    if (m_imageAvailable) vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
    if (m_renderFinished) vkDestroySemaphore(m_device, m_renderFinished, nullptr);
    if (m_inFlightFence) vkDestroyFence(m_device, m_inFlightFence, nullptr);
    if (m_commandPool) vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
}

void NativeRenderer::drawUi(const ui::UiRenderer& ui) {
    if (!m_uiPass) {
        m_uiPass = std::make_shared<ui::UiGpuPass>();
        m_uiPass->initialize(ui.font());
    }
    m_uiPass->uploadFrame(ui);
    m_uiPass->draw();
}

// ---------------------------------------------------------------------------
// Deferred path — opt-in via setDeferred(). Built lazily on the first
// endFrame() that needs it and torn down with the swapchain, so the forward
// path never pays for any of this and a resize can never leave stale-sized
// targets behind. Every failure falls back to forward rather than black.
// ---------------------------------------------------------------------------

bool NativeRenderer::ensureDeferredResources() {
    if (m_deferredReady) return true;
    if (m_deferredFailed) return false;
    if (!m_device || !m_pipelineLayout || m_swapChainImageViews.empty() ||
        m_swapChainExtent.width == 0 || m_shaderDir.empty()) return false;

    m_gbufferVertModule = createShaderModule(m_device, readFile(m_shaderDir + "/gbuffer.vert.spv"));
    m_gbufferFragModule = createShaderModule(m_device, readFile(m_shaderDir + "/gbuffer.frag.spv"));
    m_lightingVertModule = createShaderModule(m_device, readFile(m_shaderDir + "/deferred_lighting.vert.spv"));
    m_lightingFragModule = createShaderModule(m_device, readFile(m_shaderDir + "/deferred_lighting.frag.spv"));
    m_resolveFragModule = createShaderModule(m_device, readFile(m_shaderDir + "/taa.frag.spv"));
    if (!m_gbufferVertModule || !m_gbufferFragModule || !m_lightingVertModule || !m_lightingFragModule ||
        !m_resolveFragModule) {
        fprintf(stderr, "[NativeRenderer] deferred shaders missing from %s (need gbuffer.{vert,frag}.spv, "
                        "deferred_lighting.{vert,frag}.spv and taa.frag.spv) — staying on the forward path\n",
                m_shaderDir.c_str());
        m_deferredFailed = true;
        destroyDeferredResources();
        return false;
    }

    // ---- GBuffer colour targets. Sampled by the lighting pass, so they
    // carry COLOR_ATTACHMENT | SAMPLED from the start. ----
    for (int i = 0; i < kGBufferCount; ++i) {
        VkImageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = m_gbufferFormats[i];
        ci.extent = {m_swapChainExtent.width, m_swapChainExtent.height, 1};
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(m_device, &ci, nullptr, &m_gbufferImages[i]) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkMemoryRequirements mr;
        vkGetImageMemoryRequirements(m_device, m_gbufferImages[i], &mr);
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = findMemoryType(m_physicalDevice, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(m_device, &mai, nullptr, &m_gbufferMemory[i]) != VK_SUCCESS) { destroyDeferredResources(); return false; }
        vkBindImageMemory(m_device, m_gbufferImages[i], m_gbufferMemory[i], 0);

        VkImageViewCreateInfo vCi{};
        vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vCi.image = m_gbufferImages[i];
        vCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vCi.format = m_gbufferFormats[i];
        vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vCi.subresourceRange.levelCount = 1;
        vCi.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device, &vCi, nullptr, &m_gbufferViews[i]) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // Private depth: sharing m_depthImage would mean juggling its layout
    // against the forward pass, and the two never run in the same frame.
    {
        VkImageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = VK_FORMAT_D32_SFLOAT;
        ci.extent = {m_swapChainExtent.width, m_swapChainExtent.height, 1};
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (vkCreateImage(m_device, &ci, nullptr, &m_gbufferDepthImage) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkMemoryRequirements mr;
        vkGetImageMemoryRequirements(m_device, m_gbufferDepthImage, &mr);
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = findMemoryType(m_physicalDevice, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(m_device, &mai, nullptr, &m_gbufferDepthMemory) != VK_SUCCESS) { destroyDeferredResources(); return false; }
        vkBindImageMemory(m_device, m_gbufferDepthImage, m_gbufferDepthMemory, 0);

        VkImageViewCreateInfo vCi{};
        vCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vCi.image = m_gbufferDepthImage;
        vCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vCi.format = VK_FORMAT_D32_SFLOAT;
        vCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vCi.subresourceRange.levelCount = 1;
        vCi.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device, &vCi, nullptr, &m_gbufferDepthView) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // ---- Lighting output (RGBA16F so accumulated light and fog survive the
    // round trip) plus two ping-ponged temporal history slots. ----
    if (!createSampledTarget(m_physicalDevice, m_device, m_swapChainExtent.width, m_swapChainExtent.height,
                             VK_FORMAT_R16G16B16A16_SFLOAT, m_hdrImage, m_hdrMemory, m_hdrView)) {
        destroyDeferredResources();
        return false;
    }
    for (int i = 0; i < kHistoryCount; ++i) {
        if (!createSampledTarget(m_physicalDevice, m_device, m_swapChainExtent.width, m_swapChainExtent.height,
                                 VK_FORMAT_R16G16B16A16_SFLOAT, m_historyImages[i], m_historyMemory[i],
                                 m_historyViews[i])) {
            destroyDeferredResources();
            return false;
        }
    }
    m_historyIndex = 0;
    m_historyValid = false;

    // A descriptor may not name an image that is still UNDEFINED, and the
    // resolve pass only ever writes *one* history slot per frame — so both
    // need a one-time transition up front, before the first draw can legally
    // bind them (feedback is 0 that frame, so nothing is actually read yet).
    {
        VkCommandBufferAllocateInfo cbAi{};
        cbAi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbAi.commandPool = m_commandPool;
        cbAi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbAi.commandBufferCount = 1;
        VkCommandBuffer cb = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(m_device, &cbAi, &cb) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &begin);
        for (int i = 0; i < kHistoryCount; ++i) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_historyImages[i];
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
        vkEndCommandBuffer(cb);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cb;
        vkQueueSubmit(m_graphicsQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cb);
    }

    // ---- GBuffer render pass: 3 colour + depth, one subpass, and a final
    // dependency that hands the targets to the lighting pass as sampled
    // textures. ----
    {
        VkAttachmentDescription atts[4]{};
        for (int i = 0; i < 3; ++i) {
            atts[i].format = m_gbufferFormats[i];
            atts[i].samples = VK_SAMPLE_COUNT_1_BIT;
            atts[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            atts[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            atts[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            atts[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            atts[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            atts[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        atts[3].format = VK_FORMAT_D32_SFLOAT;
        atts[3].samples = VK_SAMPLE_COUNT_1_BIT;
        atts[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        atts[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        atts[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        atts[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        atts[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRefs[3];
        for (int i = 0; i < 3; ++i) colorRefs[i] = {uint32_t(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 3;
        subpass.pColorAttachments = colorRefs;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency deps[2]{};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo rpCi{};
        rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCi.attachmentCount = 4;
        rpCi.pAttachments = atts;
        rpCi.subpassCount = 1;
        rpCi.pSubpasses = &subpass;
        rpCi.dependencyCount = 2;
        rpCi.pDependencies = deps;
        if (vkCreateRenderPass(m_device, &rpCi, nullptr, &m_gbufferRenderPass) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // ---- Lighting render pass: one RGBA16F colour target (the HDR buffer),
    // no depth (fullscreen triangle), handed to the resolve pass as a
    // sampled texture. ----
    {
        VkAttachmentDescription att{};
        att.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkSubpassDependency deps[2]{};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        // Covers both sides of this pass: the GBuffer reads from the pass
        // above, and the HDR buffer's *previous* use was being sampled by
        // last frame's resolve — so its write needs that read retired too.
        deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo rpCi{};
        rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCi.attachmentCount = 1;
        rpCi.pAttachments = &att;
        rpCi.subpassCount = 1;
        rpCi.pSubpasses = &subpass;
        rpCi.dependencyCount = 2;
        rpCi.pDependencies = deps;
        if (vkCreateRenderPass(m_device, &rpCi, nullptr, &m_lightingRenderPass) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // ---- Resolve render pass: swapchain (display) + the history slot this
    // frame writes. Its external dependency is what covers *both* things the
    // resolve samples but did not write: the HDR buffer from the lighting
    // pass above, and the other history slot from the previous frame. ----
    {
        VkAttachmentDescription atts[2]{};
        atts[0].format = m_swapChainFormat;
        atts[0].samples = VK_SAMPLE_COUNT_1_BIT;
        atts[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        atts[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        atts[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        atts[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        atts[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        atts[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        atts[1].samples = VK_SAMPLE_COUNT_1_BIT;
        atts[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        atts[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        atts[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        atts[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        atts[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        atts[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRefs[2] = {
            {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
            {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        };

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 2;
        subpass.pColorAttachments = colorRefs;

        VkSubpassDependency deps[2]{};
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        // src side covers everything this pass samples but does not write:
        // the HDR buffer from the lighting pass above (color write) and the
        // history slot from the previous frame (fragment read — which is also
        // what makes overwriting the *other* slot safe, since it was sampled
        // last frame too).
        deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo rpCi{};
        rpCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCi.attachmentCount = 2;
        rpCi.pAttachments = atts;
        rpCi.subpassCount = 1;
        rpCi.pSubpasses = &subpass;
        rpCi.dependencyCount = 2;
        rpCi.pDependencies = deps;
        if (vkCreateRenderPass(m_device, &rpCi, nullptr, &m_resolveRenderPass) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // ---- Framebuffers. GBuffer and lighting are single-instance (strictly
    // one frame in flight: beginFrame() waits on the only fence), while the
    // resolve pass needs one per (history slot x swapchain image). ----
    {
        const size_t swapCount = m_swapChainImageViews.size();
        m_gbufferFramebuffers.resize(swapCount);
        for (size_t i = 0; i < swapCount; ++i) {
            VkImageView gAtts[4] = {m_gbufferViews[0], m_gbufferViews[1], m_gbufferViews[2], m_gbufferDepthView};
            VkFramebufferCreateInfo fb{};
            fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb.renderPass = m_gbufferRenderPass;
            fb.attachmentCount = 4;
            fb.pAttachments = gAtts;
            fb.width = m_swapChainExtent.width;
            fb.height = m_swapChainExtent.height;
            fb.layers = 1;
            if (vkCreateFramebuffer(m_device, &fb, nullptr, &m_gbufferFramebuffers[i]) != VK_SUCCESS) { destroyDeferredResources(); return false; }
        }

        {
            VkImageView lAtts[1] = {m_hdrView};
            VkFramebufferCreateInfo fb{};
            fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb.renderPass = m_lightingRenderPass;
            fb.attachmentCount = 1;
            fb.pAttachments = lAtts;
            fb.width = m_swapChainExtent.width;
            fb.height = m_swapChainExtent.height;
            fb.layers = 1;
            if (vkCreateFramebuffer(m_device, &fb, nullptr, &m_lightingFramebuffer) != VK_SUCCESS) { destroyDeferredResources(); return false; }
        }

        m_resolveFramebuffers.resize(kHistoryCount * swapCount);
        for (int w = 0; w < kHistoryCount; ++w) {
            for (size_t i = 0; i < swapCount; ++i) {
                VkImageView rAtts[2] = {m_swapChainImageViews[i], m_historyViews[w]};
                VkFramebufferCreateInfo fb{};
                fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb.renderPass = m_resolveRenderPass;
                fb.attachmentCount = 2;
                fb.pAttachments = rAtts;
                fb.width = m_swapChainExtent.width;
                fb.height = m_swapChainExtent.height;
                fb.layers = 1;
                const size_t slot = static_cast<size_t>(w) * swapCount + i;
                if (vkCreateFramebuffer(m_device, &fb, nullptr, &m_resolveFramebuffers[slot]) != VK_SUCCESS) { destroyDeferredResources(); return false; }
            }
        }
    }

    // ---- Lighting descriptors: FrameData UBO, shadow cascade array, and
    // the three GBuffer targets. ----
    {
        VkDescriptorSetLayoutBinding bindings[5]{};
        bindings[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        for (int i = 1; i < 5; ++i)
            bindings[i] = {uint32_t(i), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

        VkDescriptorSetLayoutCreateInfo dslCi{};
        dslCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslCi.bindingCount = 5;
        dslCi.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(m_device, &dslCi, nullptr, &m_lightingSetLayout) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkDescriptorPoolSize poolSizes[2]{};
        poolSizes[0] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
        poolSizes[1] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4};

        VkDescriptorPoolCreateInfo poolCi{};
        poolCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCi.maxSets = 1;
        poolCi.poolSizeCount = 2;
        poolCi.pPoolSizes = poolSizes;
        if (vkCreateDescriptorPool(m_device, &poolCi, nullptr, &m_lightingPool) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkDescriptorSetAllocateInfo dsAi{};
        dsAi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAi.descriptorPool = m_lightingPool;
        dsAi.descriptorSetCount = 1;
        dsAi.pSetLayouts = &m_lightingSetLayout;
        if (vkAllocateDescriptorSets(m_device, &dsAi, &m_lightingSet) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkSamplerCreateInfo sCi{};
        sCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sCi.magFilter = VK_FILTER_NEAREST;
        sCi.minFilter = VK_FILTER_NEAREST;
        sCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.maxLod = 1.0f;
        if (vkCreateSampler(m_device, &sCi, nullptr, &m_gbufferSampler) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        writeLightingDescriptorSet(m_dummyShadowView, m_dummyShadowSampler);
    }

    // ---- Resolve descriptors: FrameData, GBuffer world positions, the HDR
    // buffer and whichever history slot this frame reads. ----
    {
        VkDescriptorSetLayoutBinding bindings[4]{};
        bindings[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        for (int i = 1; i < 4; ++i)
            bindings[i] = {uint32_t(i), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

        VkDescriptorSetLayoutCreateInfo dslCi{};
        dslCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslCi.bindingCount = 4;
        dslCi.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(m_device, &dslCi, nullptr, &m_resolveSetLayout) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkDescriptorPoolSize poolSizes[2]{};
        poolSizes[0] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
        poolSizes[1] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3};

        VkDescriptorPoolCreateInfo poolCi{};
        poolCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCi.maxSets = 1;
        poolCi.poolSizeCount = 2;
        poolCi.pPoolSizes = poolSizes;
        if (vkCreateDescriptorPool(m_device, &poolCi, nullptr, &m_resolvePool) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkDescriptorSetAllocateInfo dsAi{};
        dsAi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAi.descriptorPool = m_resolvePool;
        dsAi.descriptorSetCount = 1;
        dsAi.pSetLayouts = &m_resolveSetLayout;
        if (vkAllocateDescriptorSets(m_device, &dsAi, &m_resolveSet) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        // LINEAR, unlike the GBuffer sampler: reprojection lands on
        // continuous UVs between texels, and bilinear is what turns that
        // sub-pixel offset into a smooth sample instead of a stair-step.
        VkSamplerCreateInfo sCi{};
        sCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sCi.magFilter = VK_FILTER_LINEAR;
        sCi.minFilter = VK_FILTER_LINEAR;
        sCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sCi.maxLod = 1.0f;
        if (vkCreateSampler(m_device, &sCi, nullptr, &m_hdrSampler) != VK_SUCCESS) { destroyDeferredResources(); return false; }
    }

    // ---- GBuffer pipeline: forward's vertex input and pipeline layout
    // (set0 + push constants are identical), three colour attachments. ----
    {
        VkVertexInputBindingDescription binding{0, sizeof(NativeVertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attrs[4] = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NativeVertex, px)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NativeVertex, nx)},
            {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(NativeVertex, u)},
            {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(NativeVertex, r)},
        };
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 4;
        vertexInput.pVertexAttributeDescriptions = attrs;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAtts[3]{};
        for (int i = 0; i < 3; ++i)
            blendAtts[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 3;
        colorBlend.pAttachments = blendAtts;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        VkDynamicState dynStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = 2;
        dynState.pDynamicStates = dynStates;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_gbufferVertModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_gbufferFragModule;
        stages[1].pName = "main";

        VkGraphicsPipelineCreateInfo pipeCi{};
        pipeCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeCi.stageCount = 2;
        pipeCi.pStages = stages;
        pipeCi.pVertexInputState = &vertexInput;
        pipeCi.pInputAssemblyState = &inputAssembly;
        pipeCi.pViewportState = &viewportState;
        pipeCi.pRasterizationState = &rasterizer;
        pipeCi.pMultisampleState = &multisampling;
        pipeCi.pColorBlendState = &colorBlend;
        pipeCi.pDepthStencilState = &depthStencil;
        pipeCi.pDynamicState = &dynState;
        pipeCi.layout = m_pipelineLayout;
        pipeCi.renderPass = m_gbufferRenderPass;
        pipeCi.subpass = 0;
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeCi, nullptr, &m_gbufferPipeline) != VK_SUCCESS) {
            destroyDeferredResources();
            return false;
        }
    }

    // ---- Lighting pipeline: fullscreen triangle, no vertex input, no
    // depth, single colour attachment. ----
    {
        VkDescriptorSetLayout setLayouts[1] = {m_lightingSetLayout};
        VkPipelineLayoutCreateInfo layoutCi{};
        layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCi.setLayoutCount = 1;
        layoutCi.pSetLayouts = setLayouts;
        if (vkCreatePipelineLayout(m_device, &layoutCi, nullptr, &m_lightingLayout) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAtt{};
        blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAtt;

        VkDynamicState dynStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = 2;
        dynState.pDynamicStates = dynStates;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_lightingVertModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_lightingFragModule;
        stages[1].pName = "main";

        VkGraphicsPipelineCreateInfo pipeCi{};
        pipeCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeCi.stageCount = 2;
        pipeCi.pStages = stages;
        pipeCi.pVertexInputState = &vertexInput;
        pipeCi.pInputAssemblyState = &inputAssembly;
        pipeCi.pViewportState = &viewportState;
        pipeCi.pRasterizationState = &rasterizer;
        pipeCi.pMultisampleState = &multisampling;
        pipeCi.pColorBlendState = &colorBlend;
        pipeCi.pDepthStencilState = nullptr;
        pipeCi.pDynamicState = &dynState;
        pipeCi.layout = m_lightingLayout;
        pipeCi.renderPass = m_lightingRenderPass;
        pipeCi.subpass = 0;
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeCi, nullptr, &m_lightingPipeline) != VK_SUCCESS) {
            destroyDeferredResources();
            return false;
        }
    }

    // ---- Resolve pipeline: same fullscreen vertex stage as the lighting
    // pass, but two colour attachments (swapchain + history). ----
    {
        VkDescriptorSetLayout setLayouts[1] = {m_resolveSetLayout};
        VkPipelineLayoutCreateInfo layoutCi{};
        layoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCi.setLayoutCount = 1;
        layoutCi.pSetLayouts = setLayouts;
        if (vkCreatePipelineLayout(m_device, &layoutCi, nullptr, &m_resolveLayout) != VK_SUCCESS) { destroyDeferredResources(); return false; }

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAtts[2]{};
        for (int i = 0; i < 2; ++i)
            blendAtts[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 2;
        colorBlend.pAttachments = blendAtts;

        VkDynamicState dynStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = 2;
        dynState.pDynamicStates = dynStates;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_lightingVertModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_resolveFragModule;
        stages[1].pName = "main";

        VkGraphicsPipelineCreateInfo pipeCi{};
        pipeCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeCi.stageCount = 2;
        pipeCi.pStages = stages;
        pipeCi.pVertexInputState = &vertexInput;
        pipeCi.pInputAssemblyState = &inputAssembly;
        pipeCi.pViewportState = &viewportState;
        pipeCi.pRasterizationState = &rasterizer;
        pipeCi.pMultisampleState = &multisampling;
        pipeCi.pColorBlendState = &colorBlend;
        pipeCi.pDepthStencilState = nullptr;
        pipeCi.pDynamicState = &dynState;
        pipeCi.layout = m_resolveLayout;
        pipeCi.renderPass = m_resolveRenderPass;
        pipeCi.subpass = 0;
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeCi, nullptr, &m_resolvePipeline) != VK_SUCCESS) {
            destroyDeferredResources();
            return false;
        }
    }

    m_deferredReady = true;
    return true;
}

void NativeRenderer::destroyDeferredResources() {
    if (!m_device) return;

    for (VkFramebuffer fb : m_gbufferFramebuffers) if (fb) vkDestroyFramebuffer(m_device, fb, nullptr);
    m_gbufferFramebuffers.clear();
    if (m_lightingFramebuffer) { vkDestroyFramebuffer(m_device, m_lightingFramebuffer, nullptr); m_lightingFramebuffer = VK_NULL_HANDLE; }
    for (VkFramebuffer fb : m_resolveFramebuffers) if (fb) vkDestroyFramebuffer(m_device, fb, nullptr);
    m_resolveFramebuffers.clear();

    if (m_gbufferPipeline) { vkDestroyPipeline(m_device, m_gbufferPipeline, nullptr); m_gbufferPipeline = VK_NULL_HANDLE; }
    if (m_lightingPipeline) { vkDestroyPipeline(m_device, m_lightingPipeline, nullptr); m_lightingPipeline = VK_NULL_HANDLE; }
    if (m_resolvePipeline) { vkDestroyPipeline(m_device, m_resolvePipeline, nullptr); m_resolvePipeline = VK_NULL_HANDLE; }
    if (m_lightingLayout) { vkDestroyPipelineLayout(m_device, m_lightingLayout, nullptr); m_lightingLayout = VK_NULL_HANDLE; }
    if (m_resolveLayout) { vkDestroyPipelineLayout(m_device, m_resolveLayout, nullptr); m_resolveLayout = VK_NULL_HANDLE; }

    // Shader modules can go as soon as the pipelines built from them do.
    if (m_gbufferVertModule) { vkDestroyShaderModule(m_device, m_gbufferVertModule, nullptr); m_gbufferVertModule = VK_NULL_HANDLE; }
    if (m_gbufferFragModule) { vkDestroyShaderModule(m_device, m_gbufferFragModule, nullptr); m_gbufferFragModule = VK_NULL_HANDLE; }
    if (m_lightingVertModule) { vkDestroyShaderModule(m_device, m_lightingVertModule, nullptr); m_lightingVertModule = VK_NULL_HANDLE; }
    if (m_lightingFragModule) { vkDestroyShaderModule(m_device, m_lightingFragModule, nullptr); m_lightingFragModule = VK_NULL_HANDLE; }
    if (m_resolveFragModule) { vkDestroyShaderModule(m_device, m_resolveFragModule, nullptr); m_resolveFragModule = VK_NULL_HANDLE; }

    // Destroying a pool frees every set allocated from it.
    if (m_lightingPool) { vkDestroyDescriptorPool(m_device, m_lightingPool, nullptr); m_lightingPool = VK_NULL_HANDLE; }
    m_lightingSet = VK_NULL_HANDLE;
    if (m_lightingSetLayout) { vkDestroyDescriptorSetLayout(m_device, m_lightingSetLayout, nullptr); m_lightingSetLayout = VK_NULL_HANDLE; }
    if (m_resolvePool) { vkDestroyDescriptorPool(m_device, m_resolvePool, nullptr); m_resolvePool = VK_NULL_HANDLE; }
    m_resolveSet = VK_NULL_HANDLE;
    if (m_resolveSetLayout) { vkDestroyDescriptorSetLayout(m_device, m_resolveSetLayout, nullptr); m_resolveSetLayout = VK_NULL_HANDLE; }
    if (m_gbufferSampler) { vkDestroySampler(m_device, m_gbufferSampler, nullptr); m_gbufferSampler = VK_NULL_HANDLE; }
    if (m_hdrSampler) { vkDestroySampler(m_device, m_hdrSampler, nullptr); m_hdrSampler = VK_NULL_HANDLE; }

    if (m_gbufferRenderPass) { vkDestroyRenderPass(m_device, m_gbufferRenderPass, nullptr); m_gbufferRenderPass = VK_NULL_HANDLE; }
    if (m_lightingRenderPass) { vkDestroyRenderPass(m_device, m_lightingRenderPass, nullptr); m_lightingRenderPass = VK_NULL_HANDLE; }
    if (m_resolveRenderPass) { vkDestroyRenderPass(m_device, m_resolveRenderPass, nullptr); m_resolveRenderPass = VK_NULL_HANDLE; }

    for (int i = 0; i < kGBufferCount; ++i) {
        if (m_gbufferViews[i]) { vkDestroyImageView(m_device, m_gbufferViews[i], nullptr); m_gbufferViews[i] = VK_NULL_HANDLE; }
        if (m_gbufferImages[i]) { vkDestroyImage(m_device, m_gbufferImages[i], nullptr); m_gbufferImages[i] = VK_NULL_HANDLE; }
        if (m_gbufferMemory[i]) { vkFreeMemory(m_device, m_gbufferMemory[i], nullptr); m_gbufferMemory[i] = VK_NULL_HANDLE; }
    }
    if (m_gbufferDepthView) { vkDestroyImageView(m_device, m_gbufferDepthView, nullptr); m_gbufferDepthView = VK_NULL_HANDLE; }
    if (m_gbufferDepthImage) { vkDestroyImage(m_device, m_gbufferDepthImage, nullptr); m_gbufferDepthImage = VK_NULL_HANDLE; }
    if (m_gbufferDepthMemory) { vkFreeMemory(m_device, m_gbufferDepthMemory, nullptr); m_gbufferDepthMemory = VK_NULL_HANDLE; }

    if (m_hdrView) { vkDestroyImageView(m_device, m_hdrView, nullptr); m_hdrView = VK_NULL_HANDLE; }
    if (m_hdrImage) { vkDestroyImage(m_device, m_hdrImage, nullptr); m_hdrImage = VK_NULL_HANDLE; }
    if (m_hdrMemory) { vkFreeMemory(m_device, m_hdrMemory, nullptr); m_hdrMemory = VK_NULL_HANDLE; }
    for (int i = 0; i < kHistoryCount; ++i) {
        if (m_historyViews[i]) { vkDestroyImageView(m_device, m_historyViews[i], nullptr); m_historyViews[i] = VK_NULL_HANDLE; }
        if (m_historyImages[i]) { vkDestroyImage(m_device, m_historyImages[i], nullptr); m_historyImages[i] = VK_NULL_HANDLE; }
        if (m_historyMemory[i]) { vkFreeMemory(m_device, m_historyMemory[i], nullptr); m_historyMemory[i] = VK_NULL_HANDLE; }
    }

    // Fresh history is undefined content — force the first frame after any
    // rebuild to take the current frame only.
    m_historyValid = false;
    m_historyIndex = 0;
    m_deferredReady = false;
}

void NativeRenderer::writeLightingDescriptorSet(VkImageView shadowView, VkSampler shadowSampler) {
    if (!m_lightingSet) return;

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = m_frameUBO;
    bufInfo.range = sizeof(FrameDataUBO);

    VkDescriptorImageInfo shadowInfo{};
    shadowInfo.sampler = shadowSampler;
    shadowInfo.imageView = shadowView;
    // Same reason as writeFrameDescriptorSet(): the dummy is a linear,
    // never-transitioned image, so GENERAL is the layout that actually
    // matches it rather than SHADER_READ_ONLY_OPTIMAL.
    shadowInfo.imageLayout = (shadowView == m_dummyShadowView) ? VK_IMAGE_LAYOUT_GENERAL
                                                               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo gInfo[kGBufferCount];
    for (int i = 0; i < kGBufferCount; ++i) {
        gInfo[i].sampler = m_gbufferSampler;
        gInfo[i].imageView = m_gbufferViews[i];
        gInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet writes[5]{};
    for (int i = 0; i < 5; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_lightingSet;
        writes[i].dstBinding = uint32_t(i);
        writes[i].descriptorCount = 1;
    }
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufInfo;
    for (int i = 1; i < 5; ++i) {
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = (i == 1) ? &shadowInfo : &gInfo[i - 2];
    }

    vkUpdateDescriptorSets(m_device, 5, writes, 0, nullptr);
}

void NativeRenderer::writeResolveDescriptorSet() {
    if (!m_resolveSet) return;

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = m_frameUBO;
    bufInfo.range = sizeof(FrameDataUBO);

    VkDescriptorImageInfo infos[3]{};
    infos[0].sampler = m_gbufferSampler;
    infos[0].imageView = m_gbufferViews[2];
    infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    infos[1].sampler = m_hdrSampler;
    infos[1].imageView = m_hdrView;
    infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    infos[2].sampler = m_hdrSampler;
    infos[2].imageView = m_historyViews[m_historyIndex];   // read slot; the
                                                           // write slot is
                                                           // 1 - m_historyIndex
    infos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[4]{};
    for (int i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_resolveSet;
        writes[i].dstBinding = uint32_t(i);
        writes[i].descriptorCount = 1;
    }
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &bufInfo;
    for (int i = 1; i < 4; ++i) {
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &infos[i - 1];
    }

    vkUpdateDescriptorSets(m_device, 4, writes, 0, nullptr);
}

} // namespace ks::sim
