#pragma once

#include <QString>
#include <QVector>
#include <QMap>

namespace ks {

struct MTLMaterial {
    QString name;
    float Ka[3] = {0.2f, 0.2f, 0.2f};
    float Kd[3] = {0.8f, 0.8f, 0.8f};
    float Ks[3] = {0.0f, 0.0f, 0.0f};
    float Ke[3] = {0.0f, 0.0f, 0.0f};
    float Ns = 32.0f;
    float Ni = 1.45f;
    float d = 1.0f;
    float Tr = 0.0f;
    int illum = 2;

    QString mapKd;
    QString mapKs;
    QString mapKa;
    QString mapNs;
    QString mapD;
    QString mapBump;
    QString mapDisp;
    QString mapRefl;
    QString mapKe;
    QString mapNorm;

    QMap<QString, QString> customProperties;
};

struct MTLFile {
    QString sourcePath;
    QMap<QString, MTLMaterial> materials;
};

class MaterialFormatParser {
public:
    static bool load(const QString& filePath, MTLFile& outFile);
    static bool loadFromString(const QString& content, MTLFile& outFile);
    static bool save(const QString& filePath, const MTLFile& file);

    static QString lastError() { return s_lastError; }

private:
    static QString s_lastError;
};

} // namespace ks