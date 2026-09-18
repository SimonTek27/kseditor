#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector4D>

namespace ksEditor {

// ============================================================================
// OpenGLRenderer - Motore grafico OpenGL 3.3 core per simulatore di guida
// ============================================================================
// Erede concettuale dell'architettura EngineGL33 ma completamente rewritten
// per rendering vetturale, cockpit e effetti strada. Nessuna dipendenza CWR.
class OpenGLRenderer : protected QOpenGLFunctions {
public:
    explicit OpenGLRenderer(QOpenGLWidget* parent = nullptr);
    ~OpenGLRenderer();

    // Inizializzazione
    bool initialize();
    void shutdown();

    // Cycle principale
    void renderOneFrame();

    // Dimensioni viewport
    void resizeViewport(int width, int height);

    // Matrici di proiezione e vista
    void setProjectionMatrix(const QMatrix4x4& proj);
    void setViewMatrix(const QMatrix4x4& view);

    // Stato rendering
    bool isInitialized() const { return m_initialized; }

    // Impostazioni guida
    void setFov(float fov);
    void setNearPlane(float near);
    void setFarPlane(float far);

    // Effetti rendering
    void enableFog(bool enable, const QVector3D& color, float density);
    void enableHDR(bool enable);
    void enableBloom(bool enable);

    // Luci
    void setSunDirection(const QVector3D& dir);
    void setSunColor(const QVector3D& color);
    void enableSun(bool enable);

private:
    bool createShaders();

    bool m_initialized = false;
    QOpenGLWidget* m_widget = nullptr;

    // Shader program IDs
    GLuint m_shaderProgram = 0;
    GLuint m_debugShader = 0;

    // Uniform locations
    struct Uniforms {
        QMatrix4x4 projection;
        QMatrix4x4 view;
        QMatrix4x4 model;
        GLint fogDensity;
        GLint fogColor[4];
        GLint hdrEnable;
        GLint bloomEnable;
    } m_uniforms;

    // State
    struct State {
        bool fog = false;
        QVector3D fogColor{0.7f, 0.7f, 0.8f};
        float fogDensity = 0.01f;
        bool hdr = false;
        bool bloom = false;
        QVector3D sunDirection{0, 0, 1};
        QVector3D sunColor{1, 1, 1};
        bool sunEnabled = false;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    } m_state;

    // Timer per FPS
    qint64 m_lastTime = 0;
    int m_frameCount = 0;
};

} // namespace ksEditor