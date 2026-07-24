#include "KsShaders.h"
#include <QFile>
#include <QDebug>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

namespace ks {
namespace plugins {
namespace kunos {
namespace shaders {

KsShaderManager* KsShaderManager::s_instance = nullptr;

KsShaderManager* KsShaderManager::instance() {
    if (!s_instance) {
        s_instance = new KsShaderManager();
    }
    return s_instance;
}

void KsShaderManager::initialize() {
    m_shaderDir = "shaders";
}

static QOpenGLFunctions* glFuncs() {
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    return ctx ? ctx->functions() : nullptr;
}

static void deleteProgram(GLuint prog) {
    auto* gl = glFuncs();
    if (!gl || !prog) return;
    if (gl->glIsProgram(prog)) {
        GLsizei count;
        GLuint shaders[8];
        gl->glGetAttachedShaders(prog, 8, &count, shaders);
        for (GLsizei i = 0; i < count; ++i) {
            gl->glDetachShader(prog, shaders[i]);
            gl->glDeleteShader(shaders[i]);
        }
        gl->glDeleteProgram(prog);
    }
}

void KsShaderManager::shutdown() {
    for (auto it = m_programs.begin(); it != m_programs.end(); ++it) {
        deleteProgram(it.value());
    }
    m_programs.clear();
    m_shaderInfos.clear();
    m_shaderSources.clear();
    m_uniformLocations.clear();
}

static GLuint compileShader(GLenum type, const QByteArray& source, QString* error) {
    auto* gl = glFuncs();
    if (!gl) { if (error) *error = "No OpenGL context"; return 0; }
    GLuint shader = gl->glCreateShader(type);
    if (!shader) {
        if (error) *error = "glCreateShader failed";
        return 0;
    }
    const char* src = source.constData();
    GLint len = (GLint)source.size();
    gl->glShaderSource(shader, 1, &src, &len);
    gl->glCompileShader(shader);

    GLint compiled;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint logLen;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        QByteArray log(logLen + 1, 0);
        gl->glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        if (error) *error = QString::fromUtf8(log);
        gl->glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint linkProgram(GLuint vs, GLuint fs, QString* error) {
    auto* gl = glFuncs();
    if (!gl) { if (error) *error = "No OpenGL context"; return 0; }
    GLuint prog = gl->glCreateProgram();
    if (!prog) {
        if (error) *error = "glCreateProgram failed";
        return 0;
    }
    gl->glAttachShader(prog, vs);
    gl->glAttachShader(prog, fs);
    gl->glLinkProgram(prog);

    GLint linked;
    gl->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen;
        gl->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        QByteArray log(logLen + 1, 0);
        gl->glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        if (error) *error = QString::fromUtf8(log);
        gl->glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool KsShaderManager::loadFromDirectory(const QString& path) {
    m_shaderDir = path;
    return true;
}

unsigned int KsShaderManager::getProgramId(const QString& name) const {
    return m_programs.value(name, 0);
}

bool KsShaderManager::loadProgram(const QString& name, const QString& vertexPath, const QString& fragmentPath) {
    ShaderInfo info;
    info.name = name;

    QFile vFile(vertexPath);
    if (!vFile.open(QIODevice::ReadOnly)) return false;
    QByteArray vertSrc = vFile.readAll();
    vFile.close();

    QFile fFile(fragmentPath);
    if (!fFile.open(QIODevice::ReadOnly)) return false;
    QByteArray fragSrc = fFile.readAll();
    fFile.close();

    m_shaderSources[name + "_vert"] = vertSrc;
    m_shaderSources[name + "_frag"] = fragSrc;

    QString error;
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc, &error);
    if (!vs) {
        qWarning() << "KsShaders: vertex shader" << vertexPath << "error:" << error;
        return false;
    }
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc, &error);
    if (!fs) {
        auto* gl = glFuncs();
        if (gl) gl->glDeleteShader(vs);
        qWarning() << "KsShaders: fragment shader" << fragmentPath << "error:" << error;
        return false;
    }

    GLuint prog = linkProgram(vs, fs, &error);
    if (!prog) {
        auto* gl = glFuncs();
        if (gl) { gl->glDeleteShader(vs); gl->glDeleteShader(fs); }
        qWarning() << "KsShaders: link error for" << name << ":" << error;
        return false;
    }

    if (m_programs.contains(name)) {
        deleteProgram(m_programs[name]);
    }

    m_programs[name] = prog;
    m_shaderInfos[name] = info;
    return true;
}

bool KsShaderManager::loadProgramFromSources(const QString& name, const QString& vertexSrc, const QString& fragSrc) {
    ShaderInfo info;
    info.name = name;

    m_shaderSources[name + "_vert"] = vertexSrc.toUtf8();
    m_shaderSources[name + "_frag"] = fragSrc.toUtf8();

    QString error;
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc.toUtf8(), &error);
    if (!vs) {
        qWarning() << "KsShaders: vertex source error for" << name << ":" << error;
        return false;
    }
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.toUtf8(), &error);
    if (!fs) {
        auto* gl = glFuncs();
        if (gl) gl->glDeleteShader(vs);
        qWarning() << "KsShaders: fragment source error for" << name << ":" << error;
        return false;
    }

    GLuint prog = linkProgram(vs, fs, &error);
    if (!prog) {
        auto* gl = glFuncs();
        if (gl) { gl->glDeleteShader(vs); gl->glDeleteShader(fs); }
        qWarning() << "KsShaders: link error for" << name << ":" << error;
        return false;
    }

    if (m_programs.contains(name)) {
        deleteProgram(m_programs[name]);
    }
    m_programs[name] = prog;
    m_shaderInfos[name] = info;
    return true;
}

void KsShaderManager::bindProgram(const QString& name) {
    auto* gl = glFuncs();
    if (!gl || !m_programs.contains(name)) return;
    m_currentProgram = name;
    gl->glUseProgram(m_programs[name]);
}

void KsShaderManager::releaseProgram(const QString& name) {
    auto* gl = glFuncs();
    if (!gl) return;
    if (m_currentProgram == name) {
        gl->glUseProgram(0);
        m_currentProgram.clear();
    }
}

int KsShaderManager::getUniformLocation(const QString& programName, const QString& uniformName) {
    QString key = programName + ":" + uniformName;
    if (m_uniformLocations.contains(key)) {
        return m_uniformLocations[key];
    }
    if (!m_programs.contains(programName)) {
        m_uniformLocations[key] = -1;
        return -1;
    }
    auto* gl = glFuncs();
    if (!gl) return -1;
    GLint loc = gl->glGetUniformLocation(m_programs[programName], uniformName.toUtf8().constData());
    m_uniformLocations[key] = (int)loc;
    return (int)loc;
}

QStringList KsShaderManager::getShaderNames() const {
    return m_shaderInfos.keys();
}

ShaderInfo* KsShaderManager::getShaderInfo(const QString& name) {
    if (m_shaderInfos.contains(name)) {
        return &m_shaderInfos[name];
    }
    return nullptr;
}

} // namespace shaders
} // namespace kunos
} // namespace plugins
} // namespace ks
