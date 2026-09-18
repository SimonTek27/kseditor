#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QMatrix4x4>
#include <QVector3D>

namespace ks {
namespace plugins {
namespace kunos {
namespace shaders {

struct ShaderInfo {
    QString name;
    QString fileName;
    int type = 0;
};

class KsShaderManager {
public:
    static KsShaderManager* instance();

    void initialize();
    void shutdown();

    bool loadFromDirectory(const QString& path);

    unsigned int getProgramId(const QString& name) const;
    bool loadProgram(const QString& name, const QString& vertexPath, const QString& fragmentPath);
    bool loadProgramFromSources(const QString& name, const QString& vertexSrc, const QString& fragSrc);

    void bindProgram(const QString& name);
    void releaseProgram(const QString& name);

    int getUniformLocation(const QString& programName, const QString& uniformName);
    
    QStringList getShaderNames() const;
    ShaderInfo* getShaderInfo(const QString& name);

private:
    KsShaderManager() = default;
    static KsShaderManager* s_instance;

    QMap<QString, unsigned int> m_programs;
    QMap<QString, ShaderInfo> m_shaderInfos;
    QMap<QString, QByteArray> m_shaderSources;
    QMap<QString, int> m_uniformLocations;
    QString m_currentProgram;
    QString m_shaderDir;
};

}
}
}
}