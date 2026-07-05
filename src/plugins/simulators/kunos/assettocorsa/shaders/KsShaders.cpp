#include "KsShaders.h"
#include <QFile>

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

void KsShaderManager::shutdown() {
    for (auto& prog : m_programs) {
        if (prog > 0) {
        }
    }
    m_programs.clear();
    m_shaderInfos.clear();
}

bool KsShaderManager::loadFromDirectory(const QString& path) {
    m_shaderDir = path;
    return true;
}

unsigned int KsShaderManager::getProgramId(const QString& name) const {
    return m_programs.value(name, 0);
}

bool KsShaderManager::loadProgram(const QString& name, const QString& vertexPath, const QString& fragmentPath) {
    // Read shader source files and store metadata
    ShaderInfo info;
    info.name = name;
    
    QFile vFile(vertexPath);
    if (vFile.open(QIODevice::ReadOnly)) {
        m_shaderSources[name + "_vert"] = vFile.readAll();
        vFile.close();
    }
    
    QFile fFile(fragmentPath);
    if (fFile.open(QIODevice::ReadOnly)) {
        m_shaderSources[name + "_frag"] = fFile.readAll();
        fFile.close();
    }
    
    m_shaderInfos[name] = info;
    return m_shaderSources.contains(name + "_vert") && m_shaderSources.contains(name + "_frag");
}

bool KsShaderManager::loadProgramFromSources(const QString& name, const QString& vertexSrc, const QString& fragSrc) {
    // Store shader source code directly
    ShaderInfo info;
    info.name = name;
    
    m_shaderSources[name + "_vert"] = vertexSrc.toUtf8();
    m_shaderSources[name + "_frag"] = fragSrc.toUtf8();
    m_shaderInfos[name] = info;
    return true;
}

void KsShaderManager::bindProgram(const QString& name) {
    m_currentProgram = name;
}

void KsShaderManager::releaseProgram(const QString& name) {
    m_currentProgram.clear();
}

int KsShaderManager::getUniformLocation(const QString& programName, const QString& uniformName) {
    // Cache uniform locations keyed by program+uniform
    QString key = programName + ":" + uniformName;
    if (m_uniformLocations.contains(key)) {
        return m_uniformLocations[key];
    }
    m_uniformLocations[key] = -1;
    return -1;
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