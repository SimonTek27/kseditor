#pragma once

#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>

namespace ks::sim {

// ============================================================================
// PostProcessing — Simple post-processing effects for the driving viewport
// ============================================================================
// Applies: bloom, vignette, color grading, and screen-space fog.
// Uses fullscreen quad + fragment shader approach.
// ============================================================================
class PostProcessing : protected QOpenGLExtraFunctions {
public:
    PostProcessing();
    ~PostProcessing();

    bool initialize();
    void shutdown();

    // Render effects on the current framebuffer
    // Call after the main scene is rendered
    void apply(QOpenGLFunctions* gl, int width, int height);

    // Configuration
    void setBloomEnabled(bool e) { m_bloomEnabled = e; }
    void setBloomIntensity(float i) { m_bloomIntensity = i; }
    void setVignetteIntensity(float i) { m_vignetteIntensity = i; }
    void setFogEnabled(bool e) { m_fogEnabled = e; }
    void setFogDensity(float d) { m_fogDensity = d; }
    void setFogColor(const QVector3D& c) { m_fogColor = c; }
    void setExposure(float e) { m_exposure = e; }
    void setSaturation(float s) { m_saturation = s; }

    bool isInitialized() const { return m_initialized; }

private:
    bool createShaders();
    void renderFullscreenQuad();
    void createFramebuffer(int width, int height);

    bool m_initialized = false;

    // Post-processing shader
    std::unique_ptr<QOpenGLShaderProgram> m_program;

    // Uniform locations
    GLint m_textureLoc = -1;
    GLint m_resolutionLoc = -1;
    GLint m_timeLoc = -1;
    GLint m_bloomEnabledLoc = -1;
    GLint m_bloomIntensityLoc = -1;
    GLint m_vignetteIntensityLoc = -1;
    GLint m_fogEnabledLoc = -1;
    GLint m_fogDensityLoc = -1;
    GLint m_fogColorLoc = -1;
    GLint m_exposureLoc = -1;
    GLint m_saturationLoc = -1;

    // Fullscreen quad
    GLuint m_quadVAO = 0, m_quadVBO = 0;

    // Settings
    bool m_bloomEnabled = true;
    float m_bloomIntensity = 0.3f;
    float m_vignetteIntensity = 0.4f;
    bool m_fogEnabled = true;
    float m_fogDensity = 0.001f;
    QVector3D m_fogColor{0.6f, 0.7f, 0.85f};
    float m_exposure = 1.0f;
    float m_saturation = 1.0f;

    float m_time = 0;
};

} // namespace ks::sim
