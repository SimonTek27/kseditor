#include "RenderEngine.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QPainter>
#include <QtMath>
#include <QVariant>

namespace ks {
namespace render {

RenderEngine* RenderEngine::s_instance = nullptr;

RenderEngine::RenderEngine(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
    m_settings.width = 1920;
    m_settings.height = 1080;
    m_camera.fov = 45.0f;
}

RenderEngine::~RenderEngine()
{
    shutdown();
    s_instance = nullptr;
}

RenderEngine* RenderEngine::instance()
{
    if (!s_instance) {
        s_instance = new RenderEngine();
    }
    return s_instance;
}

bool RenderEngine::initialize()
{
    qDebug() << "RenderEngine: Initializing";
    return true;
}

void RenderEngine::shutdown()
{
    m_meshes.clear();
    m_lights.clear();
    qDebug() << "RenderEngine: Shutdown complete";
}

void RenderEngine::setRenderSettings(const RenderSettings& settings)
{
    m_settings = settings;
}

void RenderEngine::setCamera(const Camera& camera)
{
    m_camera = camera;
}

void RenderEngine::addLight(const Light& light)
{
    m_lights.append(light);
    qDebug() << "RenderEngine: Added light";
}

void RenderEngine::removeLight(int index)
{
    if (index >= 0 && index < m_lights.size()) {
        m_lights.removeAt(index);
    }
}

void RenderEngine::updateLight(int index, const Light& light)
{
    if (index >= 0 && index < m_lights.size()) {
        m_lights[index] = light;
    }
}

void RenderEngine::setMeshes(const QVector<MeshData>& meshes)
{
    m_meshes = meshes;
}

void RenderEngine::addMesh(const MeshData& mesh)
{
    m_meshes.append(mesh);
}

QImage RenderEngine::render()
{
    setupPostProcessing();

    QImage image(m_settings.width, m_settings.height, QImage::Format_RGBA8888);
    image.fill(Qt::black);

    if (m_settings.engine == RenderSettings::Engine::EEVEE) {
        renderEEVEE(image);
    }

    if (m_settings.eevee.volumetric.enabled) {
        applyVolumetrics(image);
    }

    emit renderComplete(image);
    return image;
}

void RenderEngine::renderEEVEE(QImage& output)
{
    m_renderAreaX = 0;
    m_renderAreaY = 0;
    m_renderAreaWidth = output.width();
    m_renderAreaHeight = output.height();

    for (const Light& light : m_lights) {
        QColor lightColor;
        lightColor.setRgbF(light.color[0], light.color[1], light.color[2]);

        for (int y = 0; y < output.height(); ++y) {
            for (int x = 0; x < output.width(); ++x) {
                float intensity = (float)light.energy / 1000.0f;

                float nx = (float)x / output.width();
                float ny = (float)y / output.height();

                float factor = intensity * (1.0f - nx) * (1.0f - ny);

                QRgb pixel = output.pixel(x, y);
                int r = qRed(pixel) + (int)(lightColor.red() * factor);
                int g = qGreen(pixel) + (int)(lightColor.green() * factor);
                int b = qBlue(pixel) + (int)(lightColor.blue() * factor);

                output.setPixel(x, y, qRgb(qMin(255, r), qMin(255, g), qMin(255, b)));
            }
        }
    }

    if (m_settings.ambientOcclusion.enabled) {
        applyAmbientOcclusion(output);
    }

    if (m_settings.bloom.enabled) {
        QImage bloomInput = output;
        renderBloom(bloomInput, output);
    }

    if (m_settings.eevee.depthOfField.enabled) {
        applyDepthOfField(output, m_settings.eevee.depthOfField.focusDistance);
    }
}

void RenderEngine::renderBloom(const QImage& input, QImage& output)
{
    if (input.isNull()) return;
    
    // Simple bloom effect: bright-pass filter + blur composite
    QImage bloom(input.size(), QImage::Format_ARGB32);
    bloom.fill(Qt::transparent);
    
    // Extract bright areas
    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            QRgb pixel = input.pixel(x, y);
            int brightness = (qRed(pixel) + qGreen(pixel) + qBlue(pixel)) / 3;
            if (brightness > 200) {
                bloom.setPixel(x, y, pixel);
            }
        }
    }
    
    // Apply blur to bloom layer
    QImage blurred(bloom.size(), QImage::Format_ARGB32);
    for (int y = 1; y < bloom.height() - 1; ++y) {
        for (int x = 1; x < bloom.width() - 1; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    QRgb p = bloom.pixel(x + dx, y + dy);
                    rSum += qRed(p); gSum += qGreen(p);
                    bSum += qBlue(p); aSum += qAlpha(p);
                    count++;
                }
            }
            blurred.setPixel(x, y, qRgba(rSum/count, gSum/count, bSum/count, aSum/count));
        }
    }
    
    // Composite with original
    output = input.copy();
    QPainter painter(&output);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.drawImage(0, 0, blurred);
    painter.end();
}

void RenderEngine::applyAmbientOcclusion(QImage& image)
{
    if (image.isNull()) return;
    
    // Simple SSAO approximation: darken edges based on local contrast
    for (int y = 1; y < image.height() - 1; ++y) {
        for (int x = 1; x < image.width() - 1; ++x) {
            QRgb center = image.pixel(x, y);
            QRgb left = image.pixel(x - 1, y);
            QRgb right = image.pixel(x + 1, y);
            QRgb up = image.pixel(x, y - 1);
            QRgb down = image.pixel(x, y + 1);
            
            float rContrast = (qAbs(qRed(center) - qRed(left)) + qAbs(qRed(center) - qRed(right)) +
                              qAbs(qRed(center) - qRed(up)) + qAbs(qRed(center) - qRed(down))) / 1020.0f;
            float gContrast = (qAbs(qGreen(center) - qGreen(left)) + qAbs(qGreen(center) - qGreen(right)) +
                              qAbs(qGreen(center) - qGreen(up)) + qAbs(qGreen(center) - qGreen(down))) / 1020.0f;
            float bContrast = (qAbs(qBlue(center) - qBlue(left)) + qAbs(qBlue(center) - qBlue(right)) +
                              qAbs(qBlue(center) - qBlue(up)) + qAbs(qBlue(center) - qBlue(down))) / 1020.0f;
            
            float occlusion = 1.0f - (rContrast + gContrast + bContrast) * 0.3f;
            occlusion = qBound(0.7f, occlusion, 1.0f);
            
            image.setPixel(x, y, qRgba(
                qRed(center) * occlusion,
                qGreen(center) * occlusion,
                qBlue(center) * occlusion,
                qAlpha(center)
            ));
        }
    }
}

void RenderEngine::applyDepthOfField(QImage& image, float focus)
{
    if (image.isNull() || focus <= 0.0f) return;
    
    // Simple depth of field: blur based on distance from focus point
    int centerX = image.width() / 2;
    int centerY = image.height() / 2;
    float maxDist = qSqrt(centerX * centerX + centerY * centerY);
    
    QImage result = image.copy();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            float dist = qSqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
            float blurAmount = qAbs(dist / maxDist - focus) * 3.0f;
            int blurRadius = qRound(blurAmount);
            
            if (blurRadius > 0) {
                int rSum = 0, gSum = 0, bSum = 0, aSum = 0, count = 0;
                for (int dy = -blurRadius; dy <= blurRadius; ++dy) {
                    for (int dx = -blurRadius; dx <= blurRadius; ++dx) {
                        int sx = qBound(0, x + dx, image.width() - 1);
                        int sy = qBound(0, y + dy, image.height() - 1);
                        QRgb p = image.pixel(sx, sy);
                        rSum += qRed(p); gSum += qGreen(p);
                        bSum += qBlue(p); aSum += qAlpha(p);
                        count++;
                    }
                }
                if (count > 0) {
                    result.setPixel(x, y, qRgba(rSum/count, gSum/count, bSum/count, aSum/count));
                }
            }
        }
    }
    image = result;
}

void RenderEngine::applyVolumetrics(QImage& image)
{
    if (image.isNull()) return;
    
    // Simple volumetric effect: add a subtle light scattering overlay
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            float fogFactor = 0.02f; // Subtle fog
            float depth = 1.0f - (float(y) / image.height()); // Top-to-bottom gradient
            float scatter = fogFactor * depth;
            
            int r = qRed(pixel) + static_cast<int>(scatter * 50);
            int g = qGreen(pixel) + static_cast<int>(scatter * 40);
            int b = qBlue(pixel) + static_cast<int>(scatter * 30);
            
            image.setPixel(x, y, qRgba(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255), qAlpha(pixel)));
        }
    }
}

void RenderEngine::setRenderArea(int x, int y, int width, int height)
{
    m_renderAreaX = x;
    m_renderAreaY = y;
    m_renderAreaWidth = width;
    m_renderAreaHeight = height;
}

void RenderEngine::setSamples(int samples)
{
    m_settings.samples = samples;
}

ViewportRender::ViewportRender(QObject* parent)
    : QObject(parent)
    , m_shadingMode("Material")
    , m_backgroundType("World")
    , m_lightingEnabled(true)
{
    m_viewMatrix.setToIdentity();
    m_projectionMatrix.setToIdentity();
}

ViewportRender::~ViewportRender() = default;

void RenderEngine::setupLighting() {
    if (!m_lights.isEmpty()) {
        Light sun;
        for (const Light& l : m_lights) {
            if (l.lightType == Light::Type::Sun) { sun = l; break; }
        }
        if (sun.lightType != Light::Type::Sun) sun = m_lights.first();
        QVector3D lightDir = sun.rotation.normalized();
        float lightEnergy = sun.energy * sun.power;
        for (int i = 0; i < m_meshes.size(); ++i) {
            MeshData& mesh = const_cast<MeshData&>(m_meshes[i]);
            for (auto& v : mesh.vertices) {
                float NdL = QVector3D::dotProduct(v.normal, -lightDir);
                NdL = qMax(0.0f, NdL) * lightEnergy * 0.01f;
                QColor baseLight(static_cast<int>(v.color.x() * NdL * sun.color[0]),
                                 static_cast<int>(v.color.y() * NdL * sun.color[1]),
                                 static_cast<int>(v.color.z() * NdL * sun.color[2]));
                float ambient = 0.05f;
                v.color.setX(v.color.x() * ambient + baseLight.redF() * 0.5f);
                v.color.setY(v.color.y() * ambient + baseLight.greenF() * 0.5f);
                v.color.setZ(v.color.z() * ambient + baseLight.blueF() * 0.5f);
            }
        }
    }
}

void RenderEngine::setupShadows() {
    m_shadowMaps.clear();
    for (const Light& light : m_lights) {
        if (!light.castShadows || light.shadowType == Light::ShadowType::None) continue;
        int res = light.shadowResolution;
        QImage shadowMap(res, res, QImage::Format_Grayscale8);
        shadowMap.fill(255);
        for (const MeshData& mesh : m_meshes) {
            for (const auto& v : mesh.vertices) {
                float NdL = QVector3D::dotProduct(v.normal, light.rotation.normalized());
                if (NdL < 0) {
                    int sx = qBound(0, static_cast<int>((v.position.x() / 10.0f + 0.5f) * res), res - 1);
                    int sy = qBound(0, static_cast<int>((v.position.z() / 10.0f + 0.5f) * res), res - 1);
                    int sd = qBound(0, static_cast<int>((1.0f - NdL) * 255), 255);
                    if (sd < shadowMap.pixelIndex(sx, sy)) {
                        shadowMap.setPixel(sx, sy, sd);
                    }
                }
            }
        }
        m_shadowMaps.append(shadowMap);
    }
}

void RenderEngine::setupPostProcessing() {
    if (m_settings.eevee.motionBlur.enabled) {
        m_settings.useMotionBlur = true;
        m_settings.motionBlurStrength = m_settings.eevee.motionBlur.strength;
    }

    if (m_settings.eevee.reflections.enabled) {
        // m_settings reflects enabled already
    }

    if (m_settings.eevee.godRays.enabled) {
        m_settings.eevee.godRays.samples = qBound(16, m_settings.eevee.godRays.samples, 256);
    }

    if (m_settings.eevee.volumetric.enabled) {
        m_settings.eevee.volumetric.stepSize = qMax(0.01f, m_settings.eevee.volumetric.stepSize);
    }

    m_shadowMaps.clear();
    setupShadows();
    setupLighting();
}

void ViewportRender::beginFrame()
{
    m_currentFrame = QImage(1024, 768, QImage::Format_ARGB32);
    m_currentFrame.fill(Qt::darkGray);
    m_frameCount++;
}

void ViewportRender::endFrame()
{
    emit frameReady();
}

void ViewportRender::setViewportMatrix(const QMatrix4x4& matrix)
{
    m_viewMatrix = matrix;
}

void ViewportRender::setProjectionMatrix(const QMatrix4x4& matrix)
{
    m_projectionMatrix = matrix;
}

void ViewportRender::setShadingMode(const QString& mode)
{
    m_shadingMode = mode;
}

void ViewportRender::setBackgroundType(const QString& type)
{
    m_backgroundType = type;
}

void ViewportRender::setLighting(bool enabled)
{
    m_lightingEnabled = enabled;
}

void ViewportRender::update()
{
    if (m_currentFrame.isNull()) return;
    QPainter p(&m_currentFrame);
    p.setRenderHint(QPainter::Antialiasing);

    // Draw grid
    p.setPen(QPen(QColor(60, 60, 60), 1));
    for (int x = 0; x <= 1024; x += 64) {
        p.drawLine(x, 0, x, 768);
    }
    for (int y = 0; y <= 768; y += 64) {
        p.drawLine(0, y, 1024, y);
    }

    // Draw axes indicator
    int originX = 100, originY = 700;
    p.setPen(QPen(Qt::red, 3));
    p.drawLine(originX, originY, originX + 50, originY);
    p.setPen(QPen(Qt::green, 3));
    p.drawLine(originX, originY, originX, originY - 50);
    m_activeFrame = m_currentFrame;
}

void ViewportRender::cacheFrame()
{
    m_cachedFrame = m_currentFrame;
}

QVariant ViewportRender::getFrameBuffer() const
{
    return m_currentFrame.isNull() ? QVariant(m_cachedFrame) : QVariant(m_currentFrame);
}

QString EEVEESettings::toJson() const
{
    QJsonObject root;

    QJsonObject ao;
    ao["enabled"] = ambientOcclusion.enabled;
    ao["radius"] = ambientOcclusion.radius;
    ao["factor"] = ambientOcclusion.factor;
    ao["steps"] = ambientOcclusion.steps;
    ao["threshold"] = ambientOcclusion.threshold;
    ao["falloff"] = static_cast<int>(ambientOcclusion.falloff);
    root["ambientOcclusion"] = ao;

    QJsonObject bl;
    bl["enabled"] = bloom.enabled;
    bl["threshold"] = bloom.threshold;
    bl["radius"] = bloom.radius;
    bl["intensity"] = bloom.intensity;
    bl["absorption"] = bloom.absorption;
    bl["scattering"] = bloom.scattering;
    bl["quality"] = static_cast<int>(bloom.quality);
    root["bloom"] = bl;

    QJsonObject sr;
    sr["enabled"] = reflections.enabled;
    sr["roughness"] = reflections.roughness;
    sr["threshold"] = reflections.threshold;
    sr["step"] = reflections.step;
    root["screenSpaceReflections"] = sr;

    QJsonObject mb;
    mb["enabled"] = motionBlur.enabled;
    mb["strength"] = motionBlur.strength;
    mb["samples"] = static_cast<int>(motionBlur.samples);
    root["motionBlur"] = mb;

    QJsonObject dof;
    dof["enabled"] = depthOfField.enabled;
    dof["useBokeh"] = depthOfField.useBokeh;
    dof["fStop"] = depthOfField.fStop;
    dof["focalLength"] = depthOfField.focalLength;
    dof["focusDistance"] = depthOfField.focusDistance;
    dof["apertureRatio"] = depthOfField.apertureRatio;
    root["depthOfField"] = dof;

    QJsonObject vol;
    vol["enabled"] = volumetric.enabled;
    vol["density"] = volumetric.density;
    vol["absorption"] = volumetric.absorption;
    vol["scattering"] = volumetric.scattering;
    vol["anisotropy"] = volumetric.anisotropy;
    vol["lightProbes"] = volumetric.lightProbes;
    vol["stepSize"] = volumetric.stepSize;
    root["volumetrics"] = vol;

    QJsonObject sh;
    sh["cascadeSize"] = shadows.cascadeSize;
    sh["maxDistance"] = shadows.maxDistance;
    sh["bias"] = shadows.bias;
    sh["normalizedBackfaceBias"] = shadows.normalizedBackfaceBias;
    sh["biasMethod"] = static_cast<int>(shadows.biasMethod);
    root["shadows"] = sh;

    QJsonObject perf;
    perf["useIndirectScattering"] = performance.useIndirectScattering;
    perf["useSmaa"] = performance.useSmaa;
    perf["useBloom"] = performance.useBloom;
    perf["useScreenSpaceReflections"] = performance.useScreenSpaceReflections;
    perf["useLights"] = performance.useLights;
    perf["useShaderGrain"] = performance.useShaderGrain;
    perf["maxLights"] = performance.maxLights;
    perf["shadowPasLimit"] = performance.shadowPasLimit;
    perf["useBlur"] = performance.useBlur;
    root["performance"] = perf;

    QJsonObject fm;
    fm["filter"] = static_cast<int>(film.filter);
    fm["gamma"] = film.gamma;
    fm["exposure"] = film.exposure;
    fm["useDither"] = film.useDither;
    fm["dither"] = film.dither;
    fm["useToneMap"] = film.useToneMap;
    fm["toneMap"] = static_cast<int>(film.toneMap);
    fm["useClamp"] = film.useClamp;
    fm["clamp"] = film.clamp;
    root["film"] = fm;

    QJsonObject gr;
    gr["enabled"] = godRays.enabled;
    gr["density"] = godRays.density;
    gr["threshold"] = godRays.threshold;
    gr["intensity"] = godRays.intensity;
    gr["samples"] = godRays.samples;
    root["godRays"] = gr;

    return QString(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void EEVEESettings::fromJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;
    
    QJsonObject obj = doc.object();
    if (obj.contains("bloom")) {
        QJsonObject b = obj["bloom"].toObject();
        bloom.intensity = b["intensity"].toDouble(bloom.intensity);
        bloom.radius = b["radius"].toDouble(bloom.radius);
        bloom.threshold = b["threshold"].toDouble(bloom.threshold);
        bloom.enabled = b["enabled"].toBool(bloom.enabled);
    }
    if (obj.contains("ao")) {
        QJsonObject a = obj["ao"].toObject();
        ambientOcclusion.factor = a["intensity"].toDouble(ambientOcclusion.factor);
        ambientOcclusion.radius = a["radius"].toDouble(ambientOcclusion.radius);
        ambientOcclusion.enabled = a["enabled"].toBool(ambientOcclusion.enabled);
    }
    if (obj.contains("dof")) {
        QJsonObject d = obj["dof"].toObject();
        depthOfField.focusDistance = d["focalDistance"].toDouble(depthOfField.focusDistance);
        depthOfField.apertureRatio = d["aperture"].toDouble(depthOfField.apertureRatio);
        depthOfField.enabled = d["enabled"].toBool(depthOfField.enabled);
    }
    if (obj.contains("volumetrics")) {
        QJsonObject v = obj["volumetrics"].toObject();
        volumetric.density = v["density"].toDouble(volumetric.density);
        volumetric.enabled = v["enabled"].toBool(volumetric.enabled);
    }
}

}
}