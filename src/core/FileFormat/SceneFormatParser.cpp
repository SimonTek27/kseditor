#include "SceneFormatParser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {

QString SceneFormatParser::s_lastError;

bool SceneFormatParser::load(const QString& filePath, SceneFile& outScene)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        s_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &error);
    if (error.error != QJsonParseError::NoError) {
        s_lastError = "JSON parse error: " + error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        s_lastError = "Invalid scene file: root must be an object";
        return false;
    }

    QJsonObject root = doc.object();
    outScene.version = root.value("version").toString("1.0");
    outScene.name = root.value("name").toString();

    if (root.contains("metadata")) {
        QJsonObject meta = root["metadata"].toObject();
        for (auto it = meta.begin(); it != meta.end(); ++it) {
            outScene.metadata[it.key()] = it.value().toString();
        }
    }

    if (root.contains("environment")) {
        QJsonObject env = root["environment"].toObject();
        auto readVec3 = [&](const QString& key, float* dst) {
            QJsonArray arr = env.value(key).toArray();
            if (arr.size() >= 3) {
                dst[0] = arr[0].toDouble();
                dst[1] = arr[1].toDouble();
                dst[2] = arr[2].toDouble();
            }
        };

        outScene.environment.skyboxPath = env.value("skybox").toString();
        readVec3("ambientColor", outScene.environment.ambientColor);
        outScene.environment.ambientIntensity = env.value("ambientIntensity").toDouble(1.0);
        readVec3("fogColor", outScene.environment.fogColor);
        outScene.environment.fogDensity = env.value("fogDensity").toDouble(0.0);
        outScene.environment.fogStart = env.value("fogStart").toDouble(0.0);
        outScene.environment.fogEnd = env.value("fogEnd").toDouble(100.0);
        outScene.environment.fogEnabled = env.value("fogEnabled").toBool(false);
    }

    if (root.contains("nodes")) {
        QJsonArray nodesArray = root["nodes"].toArray();
        if (!nodesArray.isEmpty()) {
            outScene.rootNode = parseNode(nodesArray[0].toObject());
        }
    }

    if (root.contains("meshes")) {
        QJsonObject meshes = root["meshes"].toObject();
        for (auto it = meshes.begin(); it != meshes.end(); ++it) {
            outScene.meshData[it.key()] = it.value().toObject();
        }
    }

    if (root.contains("materials")) {
        QJsonObject materials = root["materials"].toObject();
        for (auto it = materials.begin(); it != materials.end(); ++it) {
            outScene.materialData[it.key()] = it.value().toObject();
        }
    }

    return true;
}

bool SceneFormatParser::save(const QString& filePath, const SceneFile& scene)
{
    QJsonObject root;

    root["version"] = scene.version;
    root["name"] = scene.name;

    // Metadata
    QJsonObject meta;
    for (auto it = scene.metadata.begin(); it != scene.metadata.end(); ++it) {
        meta[it.key()] = it.value();
    }
    if (!meta.isEmpty()) root["metadata"] = meta;

    // Environment
    QJsonObject env;
    env["skybox"] = scene.environment.skyboxPath;
    env["ambientColor"] = QJsonArray{scene.environment.ambientColor[0], scene.environment.ambientColor[1], scene.environment.ambientColor[2]};
    env["ambientIntensity"] = scene.environment.ambientIntensity;
    env["fogColor"] = QJsonArray{scene.environment.fogColor[0], scene.environment.fogColor[1], scene.environment.fogColor[2]};
    env["fogDensity"] = scene.environment.fogDensity;
    env["fogStart"] = scene.environment.fogStart;
    env["fogEnd"] = scene.environment.fogEnd;
    env["fogEnabled"] = scene.environment.fogEnabled;
    root["environment"] = env;

    // Nodes
    QJsonArray nodesArray;
    nodesArray.append(serializeNode(scene.rootNode));
    root["nodes"] = nodesArray;

    // Meshes
    QJsonObject meshes;
    for (auto it = scene.meshData.begin(); it != scene.meshData.end(); ++it) {
        meshes[it.key()] = it.value();
    }
    if (!meshes.isEmpty()) root["meshes"] = meshes;

    // Materials
    QJsonObject materials;
    for (auto it = scene.materialData.begin(); it != scene.materialData.end(); ++it) {
        materials[it.key()] = it.value();
    }
    if (!materials.isEmpty()) root["materials"] = materials;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        s_lastError = "Cannot write file: " + filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

SceneNode SceneFormatParser::parseNode(const QJsonObject& obj)
{
    SceneNode node;
    node.id = obj.value("id").toString();
    node.name = obj.value("name").toString();
    node.type = obj.value("type").toString("empty");
    node.visible = obj.value("visible").toBool(true);
    node.parentId = obj.value("parent").toString();
    node.meshRef = obj.value("mesh").toString();
    node.materialRef = obj.value("material").toString();

    auto readVec3 = [&](const QString& key, float* dst) {
        QJsonArray arr = obj.value(key).toArray();
        if (arr.size() == 3) { dst[0] = arr[0].toDouble(); dst[1] = arr[1].toDouble(); dst[2] = arr[2].toDouble(); }
    };

    readVec3("position", node.position);
    readVec3("rotation", node.rotation);
    readVec3("scale", node.scale);

    if (obj.contains("properties")) {
        QJsonObject props = obj["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it) {
            node.properties[it.key()] = it.value().toObject();
        }
    }

    if (obj.contains("children")) {
        QJsonArray childrenArr = obj["children"].toArray();
        for (const QJsonValue& childVal : childrenArr) {
            node.children.append(parseNode(childVal.toObject()));
        }
    }

    return node;
}

QJsonObject SceneFormatParser::serializeNode(const SceneNode& node)
{
    QJsonObject obj;
    obj["id"] = node.id;
    obj["name"] = node.name;
    obj["type"] = node.type;
    obj["visible"] = node.visible;
    if (!node.parentId.isEmpty()) obj["parent"] = node.parentId;
    if (!node.meshRef.isEmpty()) obj["mesh"] = node.meshRef;
    if (!node.materialRef.isEmpty()) obj["material"] = node.materialRef;

    obj["position"] = QJsonArray{node.position[0], node.position[1], node.position[2]};
    obj["rotation"] = QJsonArray{node.rotation[0], node.rotation[1], node.rotation[2]};
    obj["scale"] = QJsonArray{node.scale[0], node.scale[1], node.scale[2]};

    if (!node.properties.isEmpty()) {
        QJsonObject props;
        for (auto it = node.properties.begin(); it != node.properties.end(); ++it) {
            props[it.key()] = it.value();
        }
        obj["properties"] = props;
    }

    if (!node.children.isEmpty()) {
        QJsonArray childrenArr;
        for (const auto& child : node.children) {
            childrenArr.append(serializeNode(child));
        }
        obj["children"] = childrenArr;
    }

    return obj;
}

} // namespace ks