#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QImage>
#include <QMatrix4x4>
#include <QColor>
#include <QVariant>
#include "MeshOperations.h"

namespace ks {
namespace render {

struct Camera {
    QVector3D position = {0, 0, 5};
    QVector3D rotation = {0, 0, 0};
    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspect = 1.77778f;

    enum class Projection { Perspective, Orthographic };
    Projection projection = Projection::Perspective;

    enum class Type { Perspective, Orthographic, Panoramic };
    Type type = Type::Perspective;
};

struct Light {
    enum class Type { Sun, Point, Spot, Area };
    Type lightType = Type::Sun;

    QVector3D position = {0, 5, 5};
    QVector3D rotation = {0, 0, 0};

    float energy = 100.0f;
    float power = 1.0f;
    float color[3] = {1.0f, 1.0f, 1.0f};
    float temperature = 6500.0f;

    float radius = 0.1f;
    float spotSize = 0.5f;
    float spotBlend = 0.15f;

    bool useFalloff = true;

    enum class ShadowType { None, PCF, PCF16, ESM, VSM };
    ShadowType shadowType = ShadowType::PCF;
    bool castShadows = true;
    float shadowBias = 0.001f;
    float shadowOpacity = 1.0f;
    int shadowResolution = 2048;
};

struct EEVEESettings {
public:
    struct Performance {
        bool useIndirectScattering = true;
        bool useSmaa = true;
        bool useBloom = true;
        bool useScreenSpaceReflections = true;
        bool useLights = true;
        bool useShaderGrain = true;
        int maxLights = 512;
        int shadowPasLimit = 32;
        bool useBlur = false;
    } performance;

    struct AmbientOcclusion {
        bool enabled = false;
        float radius = 1.0f;
        float factor = 1.0f;
        float steps = 5;
        float threshold = 0.5f;

        enum class Falloff { Constant, Smooth, Strong };
        Falloff falloff = Falloff::Smooth;
    } ambientOcclusion;

    struct Bloom {
        bool enabled = false;
        float threshold = 1.0f;
        float radius = 0.5f;
        float intensity = 0.8f;
        float absorption = 0.6f;
        float scattering = 0.3f;

        enum class Quality { High, Medium, Low };
        Quality quality = Quality::Medium;
    } bloom;

    struct ScreenSpaceReflections {
        bool enabled = false;
        float roughness = 0.0f;
        float threshold = 0.015f;
        float step = 0.005f;
    } reflections;

    struct MotionBlur {
        bool enabled = false;
        float strength = 1.0f;

        enum class Samples { Low4 = 4, Medium8 = 8, High16 = 16 };
        Samples samples = Samples::Medium8;
    } motionBlur;

    struct DepthOfField {
        bool enabled = false;
        bool useBokeh = true;
        float fStop = 2.8f;
        float focalLength = 50.0f;
        float focusDistance = 5.0f;
        float apertureRatio = 1.0f;
    } depthOfField;

    struct Volumetric {
        bool enabled = false;
        float density = 0.0f;
        float absorption = 0.1f;
        float scattering = 0.1f;
        float anisotropy = 0.0f;
        float lightProbes = 1.0f;
        float stepSize = 0.1f;
    } volumetric;

    struct Shadows {
        int cascadeSize = 2048;
        float maxDistance = 20.0f;
        float bias = 0.001f;
        float normalizedBackfaceBias = 0.0f;
        enum class Bias { Normal, UBO };
        Bias biasMethod = Bias::Normal;
    } shadows;

    struct Film {
        enum class Filter { Fast, Medium, High };
        Filter filter = Filter::Fast;

        float gamma = 2.2f;
        float exposure = 1.0f;

        bool useDither = true;
        float dither = 0.5f;

        bool useToneMap = true;
        enum class ToneMap { Filmic, DVD, Raw };
        ToneMap toneMap = ToneMap::Filmic;

        bool useClamp = false;
        float clamp = 1.0f;
    } film;

    struct ScreenSpaceGodRays {
        bool enabled = false;
        float density = 0.3f;
        float threshold = 0.2f;
        float intensity = 1.0f;
        int samples = 64;
    } godRays;

    struct LightLinking {
        QVector<int> linkedLights;
    };

    QString toJson() const;
    void fromJson(const QString& json);
};

struct RenderSettings {
    int width = 1920;
    int height = 1080;
    float aspect = 1.77778f;

    enum class Device { CPU, GPU };
    Device device = Device::GPU;

    enum class Engine { EEVEE, CYCLES };
    Engine engine = Engine::EEVEE;

    int samples = 128;
    float sampleClamp = 8.0f;

    bool useMotionBlur = false;
    float motionBlurStrength = 1.0f;
    float motionBlurSteps = 16;
    float motionBlurCurves = 10;

    bool useDenoising = true;
    bool useAdaptiveSampling = true;

    int threads = 0;
    bool useTileSize = false;
    int tileSize = 256;

    EEVEESettings::AmbientOcclusion ambientOcclusion;
    EEVEESettings::Bloom bloom;
    EEVEESettings eevee;

    enum class Shading {
        MultiImportance,
        SingleImportance,
        BVHOnly,
        NoShadows
    };
    Shading shadowMode = Shading::MultiImportance;

    float clampDirect = 0.0f;
    float clampIndirect = 0.0f;

    float pixelFilterWidth = 1.0f;
    float filterExclude = 0.0f;

    bool useTransparent = true;
    bool useSss = true;

    enum class Priority { Background, Camera, World };
    enum class SamplingPattern { Sobol, PMJ };
};

class RenderEngine : public QObject {
    Q_OBJECT

public:
    static RenderEngine* instance();

    explicit RenderEngine(QObject* parent = nullptr);
    ~RenderEngine();

    bool initialize();
    void shutdown();

    void setRenderSettings(const RenderSettings& settings);
    RenderSettings getRenderSettings() const { return m_settings; }

    void setCamera(const Camera& camera);
    Camera getCamera() const { return m_camera; }

    void addLight(const Light& light);
    void removeLight(int index);
    QVector<Light> getLights() const { return m_lights; }
    void updateLight(int index, const Light& light);

    void setMeshes(const QVector<MeshData>& meshes);
    void addMesh(const MeshData& mesh);

    QImage render();

    void setRenderArea(int x, int y, int width, int height);
    void setSamples(int samples);

    bool isRendering() const { return m_isRendering; }

signals:
    void renderStarted();
    void renderProgress(float progress);
    void renderComplete(const QImage& image);
    void renderError(const QString& error);

private:
    static RenderEngine* s_instance;

    RenderSettings m_settings;
    Camera m_camera;
    QVector<Light> m_lights;
    QVector<MeshData> m_meshes;
    QVector<QImage> m_shadowMaps;

    bool m_isRendering = false;
    int m_renderAreaX = 0;
    int m_renderAreaY = 0;
    int m_renderAreaWidth = 1920;
    int m_renderAreaHeight = 1080;

    void renderEEVEE(QImage& output);
    void renderBloom(const QImage& input, QImage& output);
    void applyAmbientOcclusion(QImage& image);
    void applyDepthOfField(QImage& image, float focus);
    void applyVolumetrics(QImage& image);

    void setupLighting();
    void setupShadows();
    void setupPostProcessing();

    struct RenderBucket {
        int x, y, w, h;
        QImage result;
    };
    QVector<RenderBucket> m_buckets;
};

class ViewportRender : public QObject {
    Q_OBJECT

public:
    explicit ViewportRender(QObject* parent = nullptr);
    ~ViewportRender();

    void beginFrame();
    void endFrame();

    void setViewportMatrix(const QMatrix4x4& matrix);
    void setProjectionMatrix(const QMatrix4x4& matrix);

    void setShadingMode(const QString& mode);
    void setBackgroundType(const QString& type);
    void setLighting(bool enabled);

    void update();
    void cacheFrame();

    QVariant getFrameBuffer() const;

signals:
    void frameReady();

private:
    QString m_shadingMode;
    QString m_backgroundType;
    bool m_lightingEnabled;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    QImage m_currentFrame;
    QImage m_cachedFrame;
    QImage m_activeFrame;
    int m_frameCount = 0;
};

}
}