#include "XrManager.h"
#include "XrDispatch.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include <string>
#if defined(XR_VERSION_1_0) || defined(XR_NULL_HANDLE)
#include <vulkan/vulkan.h>

namespace ks {
namespace device {

bool XrManager::createSwapchains()
{
    if (m_eyeCount == 0) return false;
    if (!s_dispatch.EnumerateSwapchainFormats) return false;

    uint32_t formatCount = 0;
    s_dispatch.EnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr);
    m_swapchainFormats.resize(formatCount);
    s_dispatch.EnumerateSwapchainFormats(m_session, formatCount, &formatCount, m_swapchainFormats.data());

    m_colorFormat = 0;
    for (auto fmt : m_swapchainFormats) {
        if (fmt == VK_FORMAT_B8G8R8A8_SRGB || fmt == VK_FORMAT_R8G8B8A8_SRGB) {
            m_colorFormat = fmt;
            break;
        }
    }
    if (m_colorFormat == 0 && !m_swapchainFormats.empty())
        m_colorFormat = m_swapchainFormats[0];

    for (int i = 0; i < m_eyeCount && i < 2; i++) {
        auto& eye = m_eyes[i];
        XrSwapchainCreateInfo swapInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapInfo.format = m_colorFormat;
        swapInfo.sampleCount = 1;
        swapInfo.width = eye.swapchainImageWidth;
        swapInfo.height = eye.swapchainImageHeight;
        swapInfo.faceCount = 1;
        swapInfo.arraySize = 1;
        swapInfo.mipCount = 1;

        XrResult result = s_dispatch.CreateSwapchain(m_session, &swapInfo, &eye.swapchain);
        if (XR_FAILED(result)) return false;

        uint32_t imageCount = 0;
        s_dispatch.EnumerateSwapchainImages(eye.swapchain, 0, &imageCount, nullptr);
        std::vector<XrSwapchainImageVulkanKHR> colorImages(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
        s_dispatch.EnumerateSwapchainImages(eye.swapchain, imageCount, &imageCount,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(colorImages.data()));

        eye.colorImages.clear();
        for (const auto& img : colorImages)
            eye.colorImages.push_back(img.image);
    }
    return true;
}

void XrManager::destroySwapchains()
{
    for (int i = 0; i < 2; i++) {
        auto& eye = m_eyes[i];
        if (eye.swapchain && s_dispatch.DestroySwapchain) {
            s_dispatch.DestroySwapchain(eye.swapchain);
            eye.swapchain = XR_NULL_HANDLE;
        }
        if (eye.depthSwapchain && s_dispatch.DestroySwapchain) {
            s_dispatch.DestroySwapchain(eye.depthSwapchain);
            eye.depthSwapchain = XR_NULL_HANDLE;
        }
        eye.colorImages.clear();
        eye.depthImages.clear();
    }
}

bool XrManager::createActions()
{
    if (!s_dispatch.CreateActionSet) return true;
    XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy(setInfo.actionSetName, "game", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    strncpy(setInfo.localizedActionSetName, "Game", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    setInfo.priority = 0;
    if (XR_FAILED(s_dispatch.CreateActionSet(m_instance, &setInfo, &m_gameActionSet)))
        return false;
    return true;
}

void XrManager::destroyActions()
{
    m_gameActionSet = XR_NULL_HANDLE;
}

bool XrManager::suggestBindings() { return true; }
bool XrManager::attachActions() { return true; }

bool XrManager::pollEvents()
{
    if (!m_instance || !s_dispatch.PollEvent) return false;
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    XrResult result = s_dispatch.PollEvent(m_instance, &event);
    while (XR_SUCCEEDED(result)) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto& sessionEvent = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
            handleSessionStateChanged(sessionEvent);
        } else if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            if (onInstanceLost) onInstanceLost();
            return false;
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
        result = s_dispatch.PollEvent(m_instance, &event);
    }
    return true;
}

void XrManager::handleSessionStateChanged(const XrEventDataSessionStateChanged& event)
{
    XrSessionState oldState = m_sessionState;
    m_sessionState = event.state;
    if (onSessionStateChanged) onSessionStateChanged(oldState, event.state);

    switch (event.state) {
    case XR_SESSION_STATE_READY:
        if (s_dispatch.BeginSession) {
            XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            s_dispatch.BeginSession(m_session, &beginInfo);
            m_sessionRunning = true;
            if (onSessionRunningChanged) onSessionRunningChanged(true);
        }
        break;
    case XR_SESSION_STATE_STOPPING:
        if (s_dispatch.EndSession) s_dispatch.EndSession(m_session);
        m_sessionRunning = false;
        if (onSessionRunningChanged) onSessionRunningChanged(false);
        break;
    case XR_SESSION_STATE_FOCUSED:
        m_sessionFocused = true;
        if (onSessionFocusChanged) onSessionFocusChanged(true);
        break;
    case XR_SESSION_STATE_VISIBLE:
        m_sessionFocused = false;
        if (onSessionFocusChanged) onSessionFocusChanged(false);
        break;
    default:
        break;
    }
}

bool XrManager::pollActions() { return true; }

bool XrManager::beginXRFrame()
{
    if (!m_sessionRunning || !s_dispatch.WaitFrame || !s_dispatch.BeginFrame) return false;
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    if (XR_FAILED(s_dispatch.WaitFrame(m_session, &waitInfo, &frameState))) return false;
    if (!frameState.shouldRender) return false;
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    return XR_SUCCEEDED(s_dispatch.BeginFrame(m_session, &beginInfo));
}

bool XrManager::endXRFrame()
{
    if (!s_dispatch.EndFrame) return false;
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    return XR_SUCCEEDED(s_dispatch.EndFrame(m_session, &endInfo));
}

bool XrManager::beginEyeRender(int eyeIndex)
{
    if (eyeIndex < 0 || eyeIndex >= m_eyeCount) return false;
    auto& eye = m_eyes[eyeIndex];
    if (!eye.swapchain || !s_dispatch.AcquireSwapchainImage) return false;
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t imageIndex = 0;
    if (XR_FAILED(s_dispatch.AcquireSwapchainImage(eye.swapchain, &acquireInfo, &imageIndex))) return false;
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    if (XR_FAILED(s_dispatch.WaitSwapchainImage(eye.swapchain, &waitInfo))) return false;
    eye.currentImageIndex = static_cast<int32_t>(imageIndex);
    return true;
}

void XrManager::endEyeRender(int eyeIndex)
{
    if (eyeIndex < 0 || eyeIndex >= m_eyeCount) return;
    auto& eye = m_eyes[eyeIndex];
    if (!eye.swapchain || !s_dispatch.ReleaseSwapchainImage) return;
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    s_dispatch.ReleaseSwapchainImage(eye.swapchain, &releaseInfo);
    eye.currentImageIndex = -1;
}

XrMat4 XrManager::projectionMatrix(int eyeIndex, float nearZ, float farZ) const
{
    (void)eyeIndex;
    return XrMat4::perspective(90.f, 1.f, nearZ, farZ);
}

XrMat4 XrManager::viewMatrix(int eyeIndex) const
{
    (void)eyeIndex;
    return XrMat4::identity();
}

XrPath XrManager::stringToPath(const char* str)
{
    if (!s_dispatch.StringToPath || !m_instance) return XR_NULL_PATH;
    XrPath path = XR_NULL_PATH;
    s_dispatch.StringToPath(m_instance, str, &path);
    return path;
}

XrAction XrManager::createAction(XrActionSet actionSet, const char* name, const char* localizedName,
                                  XrActionType type, const std::vector<XrPath>& subactionPaths)
{
    if (!s_dispatch.CreateAction) return XR_NULL_HANDLE;
    XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
    strncpy(info.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    strncpy(info.localizedActionName, localizedName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    info.actionType = type;
    info.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
    info.subactionPaths = subactionPaths.empty() ? nullptr : subactionPaths.data();
    XrAction action = XR_NULL_HANDLE;
    s_dispatch.CreateAction(actionSet, &info, &action);
    return action;
}

void XrManager::suggestInteractionProfileBindings(const char* profile,
                                                   const std::vector<XrActionSuggestedBinding>& bindings)
{
    if (!s_dispatch.SuggestInteractionProfileBindings || !m_instance) return;
    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile = stringToPath(profile);
    suggested.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
    suggested.suggestedBindings = bindings.data();
    s_dispatch.SuggestInteractionProfileBindings(m_instance, &suggested);
}

} // namespace device
} // namespace ks

#endif
