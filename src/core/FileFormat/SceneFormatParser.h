#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <QMap>

namespace ks {

struct SceneNode {
    QString id;
    QString name;
    QString type; // "mesh", "light", "camera", "group", "empty"
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1};
    QMap<QString, QJsonObject> properties;
    QVector<SceneNode> children;
    QString parentId;
    QString meshRef;
    QString materialRef;
    bool visible = true;
};

struct SceneEnvironment {
    QString skyboxPath;
    float ambientColor[3] = {0.2f, 0.2f, 0.2f};
    float ambientIntensity = 1.0f;
    float fogColor[3] = {0.5f, 0.5f, 0.5f};
    float fogDensity = 0.0f;
    float fogStart = 0.0f;
    float fogEnd = 100.0f;
    bool fogEnabled = false;
};

struct SceneFile {
    QString version = "1.0";
    QString name;
    SceneNode rootNode;
    SceneEnvironment environment;
    QMap<QString, QJsonObject> meshData;
    QMap<QString, QJsonObject> materialData;
    QMap<QString, QString> metadata;
};

class SceneFormatParser {
public:
    static bool load(const QString& filePath, SceneFile& outScene);
    static bool save(const QString& filePath, const SceneFile& scene);
    static QString lastError() { return s_lastError; }

private:
    static QString s_lastError;

    static SceneNode parseNode(const QJsonObject& obj);
    static QJsonObject serializeNode(const SceneNode& node);
};

} // namespace ks