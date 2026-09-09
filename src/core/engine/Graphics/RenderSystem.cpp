#include "RenderSystem.h"
#include <QDebug>

namespace ks::engine::graphics {

bool RenderSystem::initialize(){
    if (m_initialized) return true;
    m_effects[PostEffect::Bloom] = true;
    m_effects[PostEffect::Tonemap] = true;
    m_effects[PostEffect::TAA] = false;
    m_effects[PostEffect::MSAA] = true;
    m_effects[PostEffect::MotionBlur] = false;
    m_initialized = true;
    qInfo() << "RenderSystem: initialized deferred=" << m_deferred << "vr=" << m_vr;
    return true;
}
void RenderSystem::shutdown(){
    if (!m_initialized) return;
    m_initialized = false;
    qInfo() << "RenderSystem: shutdown";
}
void RenderSystem::beginFrame(){
    runPass(RenderPass::Shadow);
    runPass(RenderPass::Geometry);
}
void RenderSystem::runPass(RenderPass p){
    emit passExecuted(p);
}
void RenderSystem::endFrame(){
    runPass(RenderPass::Post);
    emit frameReady();
}
void RenderSystem::enableEffect(PostEffect e, bool on){ m_effects[e] = on; }
void RenderSystem::setRain(float intensity, float wetness){
    m_rain.intensity = qBound(0.0f, intensity, 1.0f);
    m_rain.wetness = qBound(0.0f, wetness, 1.0f);
    m_rain.enabled = m_rain.intensity > 0.01f || m_rain.wetness > 0.01f;
    m_rain.particleCount = int(m_rain.intensity * 5000.0f);
}

} // namespace ks::engine::graphics
