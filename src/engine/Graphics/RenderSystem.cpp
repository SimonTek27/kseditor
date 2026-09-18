#include "RenderSystem.h"
#include "TerrainSystem.h"
#include "GPUParticleSystem.h"
#include "WaterSystem.h"
#include "PostProcessingPipeline.h"
#include "CascadedShadowMaps.h"
#include "VegetationSystem.h"
#include "DecalSystem.h"
#include "SSRSystem.h"
#include <QDebug>
#include <QtMath>

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
    switch (p) {
    case RenderPass::Shadow: {
        auto& csm = CascadedShadowMaps::instance();
        if (csm.isInitialized()) {
            csm.setLightDirection(m_sunDir);
            csm.update(m_sunDir, m_view, m_proj, 0.1f, 1000.0f);
        }
        break;
    }
    case RenderPass::Geometry: {
        QVector3D camPos = m_view.inverted().column(3).toVector3D();
        auto& terrain = TerrainSystem::instance();
        if (terrain.isInitialized()) {
            terrain.updateLODs(camPos);
        }
        auto& vegetation = VegetationSystem::instance();
        if (vegetation.isInitialized()) {
            vegetation.update(0.016f, camPos, m_sunDir);
        }
        auto& water = WaterSystem::instance();
        if (water.isInitialized()) {
            water.update(0.016f, camPos);
        }
        auto& particles = GPUParticleSystem::instance();
        if (particles.isInitialized()) {
            particles.update(0.016f);
            particles.render(m_view, m_proj, camPos);
        }
        auto& decals = DecalSystem::instance();
        if (decals.isInitialized()) {
            decals.update(0.016f, camPos);
        }
        break;
    }
    case RenderPass::Post: {
        auto& pp = PostProcessingPipeline::instance();
        if (pp.isInitialized()) {
            if (m_effects[PostEffect::Bloom]) pp.enableBloom(true);
            else pp.enableBloom(false);
            if (m_effects[PostEffect::MotionBlur]) pp.enableMotionBlur(true);
            else pp.enableMotionBlur(false);
        }
        auto& ssr = SSRSystem::instance();
        if (ssr.isInitialized()) {
            ssr.configure(SSRConfig{});
        }
        break;
    }
    }
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

    auto& particles = GPUParticleSystem::instance();
    if (particles.isInitialized() && m_rain.enabled) {
        particles.setGravity(QVector3D(0, -9.81f, 0));
        particles.setWind(m_sunDir * 0.5f);
    }
}

} // namespace ks::engine::graphics
