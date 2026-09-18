#include "PostProcessing.h"
#include <QDebug>
#include <QtMath>
#include <cmath>

namespace ks::sim {

PostProcessing::PostProcessing() = default;
PostProcessing::~PostProcessing() { shutdown(); }

bool PostProcessing::initialize()
{
    if (m_initialized) return true;

    initializeOpenGLFunctions();

    if (!createShaders()) {
        qWarning() << "PostProcessing: Failed to create shaders";
        return false;
    }

    // Fullscreen quad vertices: pos(3) + uv(2)
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    m_initialized = true;
    qInfo() << "PostProcessing: Initialized";
    return true;
}

void PostProcessing::shutdown()
{
    if (!m_initialized) return;
    if (m_program) m_program.reset();
    if (m_quadVAO) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    m_initialized = false;
}

bool PostProcessing::createShaders()
{
    const char* vertSrc = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D uTexture;
        uniform vec2 uResolution;
        uniform float uTime;
        uniform bool uBloomEnabled;
        uniform float uBloomIntensity;
        uniform float uVignetteIntensity;
        uniform bool uFogEnabled;
        uniform float uFogDensity;
        uniform vec3 uFogColor;
        uniform float uExposure;
        uniform float uSaturation;

        vec3 adjustSaturation(vec3 color, float adjustment) {
            float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
            return mix(vec3(luminance), color, adjustment);
        }

        void main() {
            vec3 color = texture(uTexture, vTexCoord).rgb;

            // Simple bloom (bright pass + blur approximation)
            if (uBloomEnabled) {
                float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
                if (brightness > 0.8) {
                    vec3 bloom = color * (brightness - 0.8) * uBloomIntensity * 2.0;
                    color += bloom;
                }
            }

            // Vignette
            vec2 uv = vTexCoord;
            float dist = length(uv - 0.5);
            float vignette = smoothstep(0.7, 0.3, dist);
            color *= mix(1.0, vignette, uVignetteIntensity);

            // Fog
            if (uFogEnabled) {
                float depth = texture(uTexture, vTexCoord).r;
                float fogFactor = 1.0 - exp(-uFogDensity * depth * 1000.0);
                fogFactor = clamp(fogFactor, 0.0, 1.0);
                color = mix(color, uFogColor, fogFactor * 0.3);
            }

            // Exposure
            color *= uExposure;

            // Saturation
            color = adjustSaturation(color, uSaturation);

            // Tone mapping (simple Reinhard)
            color = color / (color + vec3(1.0));

            FragColor = vec4(color, 1.0);
        }
    )";

    m_program = std::make_unique<QOpenGLShaderProgram>();
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc)) {
        qWarning() << "PostProcessing: Vertex shader failed:" << m_program->log();
        return false;
    }
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
        qWarning() << "PostProcessing: Fragment shader failed:" << m_program->log();
        return false;
    }
    if (!m_program->link()) {
        qWarning() << "PostProcessing: Shader link failed:" << m_program->log();
        return false;
    }

    m_textureLoc = m_program->uniformLocation("uTexture");
    m_resolutionLoc = m_program->uniformLocation("uResolution");
    m_timeLoc = m_program->uniformLocation("uTime");
    m_bloomEnabledLoc = m_program->uniformLocation("uBloomEnabled");
    m_bloomIntensityLoc = m_program->uniformLocation("uBloomIntensity");
    m_vignetteIntensityLoc = m_program->uniformLocation("uVignetteIntensity");
    m_fogEnabledLoc = m_program->uniformLocation("uFogEnabled");
    m_fogDensityLoc = m_program->uniformLocation("uFogDensity");
    m_fogColorLoc = m_program->uniformLocation("uFogColor");
    m_exposureLoc = m_program->uniformLocation("uExposure");
    m_saturationLoc = m_program->uniformLocation("uSaturation");

    return true;
}

void PostProcessing::apply(QOpenGLFunctions* gl, int width, int height)
{
    if (!m_initialized || !m_program) return;

    m_program->bind();
    m_program->setUniformValue(m_resolutionLoc, QVector2D(width, height));
    m_program->setUniformValue(m_timeLoc, m_time);
    m_program->setUniformValue(m_bloomEnabledLoc, m_bloomEnabled);
    m_program->setUniformValue(m_bloomIntensityLoc, m_bloomIntensity);
    m_program->setUniformValue(m_vignetteIntensityLoc, m_vignetteIntensity);
    m_program->setUniformValue(m_fogEnabledLoc, m_fogEnabled);
    m_program->setUniformValue(m_fogDensityLoc, m_fogDensity);
    m_program->setUniformValue(m_fogColorLoc, m_fogColor);
    m_program->setUniformValue(m_exposureLoc, m_exposure);
    m_program->setUniformValue(m_saturationLoc, m_saturation);

    gl->glActiveTexture(GL_TEXTURE0);
    m_program->setUniformValue(m_textureLoc, 0);

    renderFullscreenQuad();
    m_program->release();

    m_time += 0.016f;
}

void PostProcessing::renderFullscreenQuad()
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessing::createFramebuffer(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    // Framebuffer creation is handled externally by the DrivingViewport
}

} // namespace ks::sim
