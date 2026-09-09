#include "OpenGLRenderer.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>
#include <QDebug>
#include <QFile>

namespace ksEditor {

OpenGLRenderer::OpenGLRenderer(QOpenGLWidget* parent) : QOpenGLFunctions(), m_widget(parent) {}

OpenGLRenderer::~OpenGLRenderer() {
    shutdown();
}

bool OpenGLRenderer::initialize() {
    if (m_initialized) return true;

    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!createShaders()) {
        qWarning() << "OpenGLRenderer: Failed to create shaders";
        return false;
    }

    m_initialized = true;
    qInfo() << "OpenGLRenderer: Initialization complete";
    return true;
}

void OpenGLRenderer::shutdown() {
    if (!m_initialized) return;

    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_debugShader) {
        glDeleteProgram(m_debugShader);
        m_debugShader = 0;
    }

    m_initialized = false;
}

void OpenGLRenderer::renderOneFrame() {
    if (!m_initialized) return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_shaderProgram);
}

void OpenGLRenderer::resizeViewport(int width, int height) {
    if (!m_initialized) return;
    glViewport(0, 0, width, height);
}

void OpenGLRenderer::setProjectionMatrix(const QMatrix4x4& proj) {
    m_uniforms.projection = proj;
}

void OpenGLRenderer::setViewMatrix(const QMatrix4x4& view) {
    m_uniforms.view = view;
}

void OpenGLRenderer::setFov(float fov) {
    m_state.fov = qBound(10.0f, fov, 170.0f);
    if (m_initialized && m_widget) {
        float aspect = m_widget->width() > 0 ? float(m_widget->width()) / float(m_widget->height()) : 16.0f / 9.0f;
        QMatrix4x4 proj;
        proj.perspective(m_state.fov, aspect, m_state.nearPlane, m_state.farPlane);
        m_uniforms.projection = proj;
    }
}
void OpenGLRenderer::setNearPlane(float near) {
    m_state.nearPlane = qMax(0.01f, near);
    if (m_initialized && m_widget) {
        float aspect = m_widget->width() > 0 ? float(m_widget->width()) / float(m_widget->height()) : 16.0f / 9.0f;
        QMatrix4x4 proj;
        proj.perspective(m_state.fov, aspect, m_state.nearPlane, m_state.farPlane);
        m_uniforms.projection = proj;
    }
}
void OpenGLRenderer::setFarPlane(float far) {
    m_state.farPlane = qMax(m_state.nearPlane + 1.0f, far);
    if (m_initialized && m_widget) {
        float aspect = m_widget->width() > 0 ? float(m_widget->width()) / float(m_widget->height()) : 16.0f / 9.0f;
        QMatrix4x4 proj;
        proj.perspective(m_state.fov, aspect, m_state.nearPlane, m_state.farPlane);
        m_uniforms.projection = proj;
    }
}

void OpenGLRenderer::enableFog(bool enable, const QVector3D& color, float density) {
    m_state.fog = enable;
    m_state.fogColor = color;
    m_state.fogDensity = qBound(0.0f, density, 1.0f);
    if (m_initialized && m_shaderProgram) {
        glUseProgram(m_shaderProgram);
        GLint loc = glGetUniformLocation(m_shaderProgram, "fogEnabled");
        if (loc >= 0) glUniform1i(loc, enable ? 1 : 0);
        loc = glGetUniformLocation(m_shaderProgram, "fogColor");
        if (loc >= 0) glUniform3f(loc, color.x(), color.y(), color.z());
        loc = glGetUniformLocation(m_shaderProgram, "fogDensity");
        if (loc >= 0) glUniform1f(loc, m_state.fogDensity);
    }
}

void OpenGLRenderer::enableHDR(bool enable) { m_state.hdr = enable; }
void OpenGLRenderer::enableBloom(bool enable) { m_state.bloom = enable; }

void OpenGLRenderer::setSunDirection(const QVector3D& dir) {
    m_state.sunDirection = dir.normalized();
}

void OpenGLRenderer::setSunColor(const QVector3D& color) {
    m_state.sunColor = color;
}

void OpenGLRenderer::enableSun(bool enable) {
    m_state.sunEnabled = enable;
}

bool OpenGLRenderer::createShaders() {
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "out vec3 FragPos;\n"
        "out vec3 Normal;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "   TexCoord = aTexCoord;\n"
        "}";

    const char* fragmentShaderSource = "#version 330 core\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 sunDirection;\n"
        "uniform vec3 sunColor;\n"
        "uniform bool sunEnabled;\n"
        "void main() {\n"
        "   vec3 norm = normalize(Normal);\n"
        "   float diff = max(dot(norm, vec3(0.0, 0.0, 1.0)), 0.0);\n"
        "   vec3 ambient = vec3(0.05);\n"
        "   vec3 diffuse = diff * vec3(0.8);\n"
        "   vec3 result = ambient + diffuse;\n"
        "   if (sunEnabled) {\n"
        "       float diffuseSun = max(dot(norm, normalize(sunDirection)), 0.0);\n"
        "       result += diffuseSun * sunColor;\n"
        "   }\n"
        "   FragColor = vec4(result, 1.0);\n"
        "}";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        qWarning() << "VERTEX SHADER ERROR:" << QString::fromUtf8(infoLog);
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        qWarning() << "FRAGMENT SHADER ERROR:" << QString::fromUtf8(infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    GLint linkSuccess;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        qWarning() << "SHADER LINK ERROR:" << QString::fromUtf8(infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

} // namespace ksEditor
